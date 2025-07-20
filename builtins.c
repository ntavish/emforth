/**
 * @file builtins.c
 *
 * @brief This file contains the definitions of the built-in words
 * used by this forth.
 */
#include "builtins.h"
#include "builtins_common.h"
#include "emforth.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

/* forward declarations of primitive word functions also used by other words */
void do_word(struct forth_ctx *ctx);
void do_2dfa(struct forth_ctx *ctx);

/* === Helper functions === */
/**
 * @brief pushes string to stack, followed by the length as top of stack
 * Note that string will be pushed will padding if necessary.
 */
int stack_push_wordname(struct forth_ctx *ctx, char *s, int len)
{
	/* maximum 32 len */
	len = len % (WORD_NAME_MAX_LEN - 1);
	/* round up to 4 byte boundary */
	stack_cell_t r_len = ALIGN_UP_WORD_T(len);

	/* clear the to-be-written-to area */
	memset(&ctx->stack[ctx->sp], 0, r_len);

	/* copy to stack and adjust stack */
	memcpy(&ctx->stack[ctx->sp], s, len);
	stack_add(ctx, r_len / sizeof(stack_cell_t));
	stack_push(ctx, len);

	return len;
}

/* Helper function to find a word's name from its execution token */
static const char *find_word_name_by_xt(struct forth_ctx *ctx, word_t xt)
{
	/**
	 * TODO: this might not need traversal..
	 */
	dict_header_t *header = ctx->dict.latest;
	while (header != DICT_NULL) {
		char *word_name = (char *)(header + 1);
		word_t *codeword_addr =
		    (word_t *)(word_name +
			       ALIGN_UP_WORD_T(header->flags.f.length));
		word_t codeword = *codeword_addr;

		if (codeword == do_docol) {
			/* For a colon word, compare the CFA */
			if ((word_t)codeword_addr == xt) {
				return word_name;
			}
		} else {
			/* For a primitive, compare the function pointer */
			if (codeword == xt) {
				return word_name;
			}
		}
		header = header->link;
	}
	return NULL;
}

/* helper function to print word defintion from dictionary header */
void print_word_def(struct forth_ctx *ctx)
{
	dict_header_t *header = (dict_header_t *) stack_pop(ctx);
	char buf[MAX_INPUT_LEN + 1];

	char *word_name = (char *)(header + 1);
	word_t *cfa =
	    (word_t *)(word_name + ALIGN_UP_WORD_T(header->flags.f.length));

	if (header->flags.f.hidden) {
		return;
	}

	memcpy(buf, word_name, header->flags.f.length);
	buf[header->flags.f.length] = 0;
	ctx->plat.puts(": ");
	ctx->plat.puts(buf);
	ctx->plat.puts(" ");

	if (header->flags.f.immediate) {
		ctx->plat.puts("immediate ");
	}

	if (*cfa != do_docol) {
		ctx->plat.puts("[primitive]\n");
		return;
	}

	word_t *ip = cfa + 1;
	while (*ip != do_exit) {
		const char *word_name = find_word_name_by_xt(ctx, *ip);
		if (word_name) {
			ctx->plat.puts(word_name);
			ctx->plat.puts(" ");
		} else {
			char buf[32];
			snprintf(buf, sizeof(buf), "%lu ", (unsigned long)*ip);
			ctx->plat.puts(buf);
		}
		ip++;
	}

	ctx->plat.puts(";\n");
}

/*=== primitive words functions follow ===*/

/**
 * @brief creates new dictionaly item
 *
 * stack_pop(ctx) -> length of the word to be created
 * STACK_SUB(ALIGN_UP_WORD_T(len/4)) -> the word to be created
 */
void do_create_word(struct forth_ctx *ctx)
{
	dict_header_t *old_latest;
	dict_header_t *new;

	old_latest = ctx->dict.latest;
	ctx->dict.here = (unsigned char *)ALIGN_UP_WORD_T(ctx->dict.here);
	new = (dict_header_t *)ctx->dict.here;
	ctx->dict.here = (unsigned char *)(new + 1);

	new->link = old_latest;
	new->flags.f.hidden = 0;
	new->flags.f.immediate = 0;
	new->flags.f.length = stack_pop(ctx);

	stack_sub(ctx, ALIGN_UP_WORD_T(new->flags.f.length) / (sizeof(word_t)));
	memset(ctx->dict.here, 0, ALIGN_UP_WORD_T(new->flags.f.length));
	memcpy(ctx->dict.here, &ctx->stack[ctx->sp], new->flags.f.length);

	/**
	 * add length to 'here' round up to word_t size for alignment.
	 */
	ctx->dict.here += new->flags.f.length;
	ctx->dict.here =
	    (unsigned char *)ALIGN_UP_WORD_T((stack_cell_t)ctx->dict.here);
	ctx->dict.latest = new;
}

void do_find(struct forth_ctx *ctx)
{
	stack_cell_t len = stack_pop(ctx);
	size_t num_cells = ALIGN_UP_WORD_T(len) / sizeof(word_t);
	char *addr = (char *)&ctx->stack[ctx->sp - num_cells];
	stack_sub(ctx, num_cells);
	stack_push(ctx, (stack_cell_t)find_word_header(ctx, addr, len));
}

void do_tick(struct forth_ctx *ctx)
{
	/* push the xt of the next word to stack */
	stack_push(ctx, (stack_cell_t)*ctx->ip);
	/* Skip over the next word */
	ctx->ip += 1;
}

/**
 * @brief creates new dictionaly item
 *
 * stack_pop(ctx) -> the codeword to be appended
 * ctx->dict.here += 4 after appending to current word definition
 */
void do_comma(struct forth_ctx *ctx)
{
	stack_cell_t codeword = stack_pop(ctx);
	compile_word(ctx, codeword);
}

void do_printstack(struct forth_ctx *ctx)
{
	ctx->plat.puts("STACK > ");
	stack_cell_t p = ctx->sp;
	char str[32];
	while (p > 0 && p <= STACK_SIZE_MAX) {
		if (p - 1 < STACK_SIZE_MAX) {
			snprintf(str, sizeof(str), "%lu ", ctx->stack[p - 1]);
		} else {
			snprintf(str, sizeof(str), "??? ");
		}
		str[sizeof(str) - 1] = 0;
		ctx->plat.puts(str);
		p--;
	}
	ctx->plat.puts("\n");
}

void do_dot(struct forth_ctx *ctx)
{
	char str[30];
	if (ctx->sp > 0) {
		snprintf(str, sizeof(str), "%lu\n", stack_pop(ctx));
		str[sizeof(str) - 1] = 0;
		ctx->plat.puts(str);
	} else {
		snprintf(str, sizeof(str), "Data stack underflow\n");
		str[sizeof(str) - 1] = 0;
		ctx->plat.puts(str);
	}
}

/**
 * @brief drops top item from stack
 */
void do_drop(struct forth_ctx *ctx)
{
	if (ctx->sp > 0) {
		(void)stack_pop(ctx);
	}
}

/**
 * @brief duplicates top item on stack
 */
void do_dup(struct forth_ctx *ctx)
{
	if (ctx->sp >= 1) {
		stack_cell_t n1 = stack_pop(ctx);
		stack_push(ctx, n1);
		stack_push(ctx, n1);
	}
}

/**
 * @brief swaps top two items on stack
 */
void do_swap(struct forth_ctx *ctx)
{
	if (ctx->sp >= 2) {
		stack_cell_t n1 = stack_pop(ctx);
		stack_cell_t n2 = stack_pop(ctx);
		stack_push(ctx, n1);
		stack_push(ctx, n2);
	}
}

/**
 * @brief rotates top three items on stack
 */
void do_rot(struct forth_ctx *ctx)
{
	if (ctx->sp >= 3) {
		stack_cell_t n1 = stack_pop(ctx);
		stack_cell_t n2 = stack_pop(ctx);
		stack_cell_t n3 = stack_pop(ctx);
		stack_push(ctx, n2);
		stack_push(ctx, n1);
		stack_push(ctx, n3);
	}
}

/**
 * @brief copies second item to top of stack
 */
void do_over(struct forth_ctx *ctx)
{
	if (ctx->sp >= 2) {
		stack_cell_t n1 = stack_pop(ctx);
		stack_cell_t n2 = stack_pop(ctx);
		stack_push(ctx, n2);
		stack_push(ctx, n1);
		stack_push(ctx, n2);
	}
}

void do_plus(struct forth_ctx *ctx)
{
	stack_cell_t n1 = stack_pop(ctx);
	stack_cell_t n2 = stack_pop(ctx);
	stack_push(ctx, n1 + n2);
}

void do_multiply(struct forth_ctx *ctx)
{
	stack_cell_t n1 = stack_pop(ctx);
	stack_cell_t n2 = stack_pop(ctx);
	stack_push(ctx, n1 * n2);
}

void do_minus(struct forth_ctx *ctx)
{
	stack_cell_t n1 = stack_pop(ctx);
	stack_cell_t n2 = stack_pop(ctx);
	stack_push(ctx, n2 - n1);
}

void do_divide(struct forth_ctx *ctx)
{
	stack_cell_t n1 = stack_pop(ctx);
	stack_cell_t n2 = stack_pop(ctx);
	if (n1 != 0) {
		stack_push(ctx, n2 / n1);
	} else {
		ctx->plat.puts("Division by zero error\n");
		stack_push(ctx, 0);
	}
}

/**
 * @brief pops top two items on stack and pushes result of their modulo.
 */
void do_mod(struct forth_ctx *ctx)
{
	stack_cell_t n1 = stack_pop(ctx);
	stack_cell_t n2 = stack_pop(ctx);
	if (n1 != 0) {
		stack_push(ctx, n2 % n1);
	} else {
		ctx->plat.puts("Division by zero error\n");
		stack_push(ctx, 0);
	}
}

/**
 * @brief increments top stack item by 1.
 */
void do_incr(struct forth_ctx *ctx)
{
	if (ctx->sp > 0) {
		/* sp is next available slot */
		ctx->stack[ctx->sp - 1]++;
	}
}

/**
 * @brief decrements top stack item by 1.
 */
void do_decr(struct forth_ctx *ctx)
{
	if (ctx->sp > 0) {
		/* sp is next available slot */
		ctx->stack[ctx->sp - 1]--;
	}
}

/**
 * @brief compares top two items on stack for equality.
 */
void do_equal(struct forth_ctx *ctx)
{
	if (ctx->sp >= 2) {
		stack_cell_t n1 = stack_pop(ctx);
		stack_cell_t n2 = stack_pop(ctx);
		stack_push(ctx, n1 == n2 ? 1 : 0);
	} else {
		stack_push(ctx, 0);
	}
}

/**
 * @brief compares top two items on stack (n1 n2 -- flag), flag is true if n1 <
 * n2.
 */
void do_less_than(struct forth_ctx *ctx)
{
	if (ctx->sp >= 2) {
		stack_cell_t n1 = stack_pop(ctx);
		stack_cell_t n2 = stack_pop(ctx);
		stack_push(ctx, n2 < n1 ? 1 : 0);
	} else {
		stack_push(ctx, 0);
	}
}

/**
 * @brief compares top two items on stack (n1 n2 -- flag), flag is true if n1 >
 * n2.
 */
void do_greater_than(struct forth_ctx *ctx)
{
	if (ctx->sp >= 2) {
		stack_cell_t n1 = stack_pop(ctx);
		stack_cell_t n2 = stack_pop(ctx);
		stack_push(ctx, n2 > n1 ? 1 : 0);
	} else {
		stack_push(ctx, 0);
	}
}

/**
 * @brief tests if top stack item equals zero.
 */
void do_zero_equal(struct forth_ctx *ctx)
{
	if (ctx->sp >= 1) {
		stack_cell_t n = stack_pop(ctx);
		stack_push(ctx, n == 0 ? 1 : 0);
	} else {
		stack_push(ctx, 0);
	}
}

/**
 * @brief fetches a 32-bit value from memory at given address.
 */
void do_fetch(struct forth_ctx *ctx)
{
	if (ctx->sp > 0) {
		stack_cell_t addr = stack_pop(ctx);
		/* Check if address is within dictionary bounds */
		if ((void *)addr >= (void *)ctx->dict.mem &&
		    ((void *)addr + sizeof(stack_cell_t)) <=
			(void *)(ctx->dict.mem + DICTIONARY_MEMORY_SIZE)) {
			/* Use raw address directly */
			stack_cell_t value = *(stack_cell_t *)addr;
			stack_push(ctx, value);
		} else {
			ctx->plat.puts(__func__);
			ctx->plat.puts(
			    ": error - accessing outside dictionary bounds\n");
			stack_push(ctx, 0); /* Push a default value on error */
		}
	} else {
		stack_push(ctx, 0);
	}
}

/**
 * @brief stores a 32-bit value to memory at given address.
 */
void do_store(struct forth_ctx *ctx)
{
	if (ctx->sp >= 2) {
		stack_cell_t addr = stack_pop(ctx);
		stack_cell_t value = stack_pop(ctx);
		/* Check if address is within dictionary bounds */
		if ((void *)addr >= (void *)ctx->dict.mem &&
		    ((void *)addr + sizeof(stack_cell_t)) <=
			(void *)(ctx->dict.mem + DICTIONARY_MEMORY_SIZE)) {
			/* Use raw address directly */
			*(stack_cell_t *)addr = value;
		} else {
			ctx->plat.puts(__func__);
			ctx->plat.puts(
			    ": error - accessing outside dictionary bounds\n");
		}
	}
}

/**
 * @brief fetches a single byte from memory at given address.
 */
void do_cfetch(struct forth_ctx *ctx)
{
	if (ctx->sp > 0) {
		stack_cell_t addr = stack_pop(ctx);
		/* Check if address is within dictionary bounds and warn if not
		 */
		if ((void *)addr >= (void *)ctx->dict.mem &&
		    (void *)addr <
			(void *)(ctx->dict.mem + DICTIONARY_MEMORY_SIZE)) {
			/* Address is within dictionary */
		} else {
			ctx->plat.puts(__func__);
			ctx->plat.puts(": warning - accessing outside "
				       "dictionary bounds\n");
		}
		/* Use raw address directly */
		unsigned char value = *(unsigned char *)addr;
		stack_push(ctx, value);
	} else {
		ctx->plat.puts(__func__);
		ctx->plat.puts(" stack underflow\n");
		stack_push(ctx, 0);
	}
}

/**
 * @brief stores a single byte to memory at given address.
 */
void do_cstore(struct forth_ctx *ctx)
{
	if (ctx->sp >= 2) {
		stack_cell_t addr = stack_pop(ctx);
		stack_cell_t value = stack_pop(ctx);
		/* Check if address is within dictionary bounds and warn if not
		 */
		if ((void *)addr >= (void *)ctx->dict.mem &&
		    (void *)addr <
			(void *)(ctx->dict.mem + DICTIONARY_MEMORY_SIZE)) {
			/* Address is within dictionary */
		} else {
			ctx->plat.puts(__func__);
			ctx->plat.puts(": warning - accessing outside "
				       "dictionary bounds\n");
		}
		/* Use raw address directly */
		*(unsigned char *)addr = (unsigned char)(value & 0xFF);
	}
}

/**
 * @brief convert pointer to word in dictionary to the code field address.
 * stack_pop(ctx) -> dictionary word pointer
 * ----
 * stack_push(ctx, address)
 */
void do_2cfa(struct forth_ctx *ctx)
{
	dict_header_t *w_h = (dict_header_t *)stack_pop(ctx);
	stack_push(ctx, (stack_cell_t)(w_h + 1));
}

/**
 * @brief this one gives the same address as would be compiled for a word
 * which is included in the definition of a forth word. We also call it xt
 * or execution token.
 *
 * stack_pop(ctx) -> dictionary word pointer
 * ----
 * stack_push(ctx, address)
 */
void do_2dfa(struct forth_ctx *ctx)
{
	dict_header_t *w_h = (dict_header_t *)stack_pop(ctx);
	char *word_name = (char *)(w_h + 1);
	word_t *codeword_addr =
	    (word_t *)(word_name + ALIGN_UP_WORD_T(w_h->flags.f.length));
	word_t codeword = *codeword_addr;

	if (codeword == do_docol) {
		/* For a colon word, the xt is the address of its definition
		 * (the CFA). */
		stack_push(ctx, (stack_cell_t)codeword_addr);
	} else {
		/* For a primitive, the xt is the function pointer itself (the
		 * code). */
		stack_push(ctx, (stack_cell_t)codeword);
	}
}

/**
 * @brief switch to immediatemode
 */
void do_lbrac(struct forth_ctx *ctx)
{
	ctx->intrp_data.mode = MODE_IMMEDIATE;
}

/**
 * @brief switch to compile mode
 */
void do_rbrac(struct forth_ctx *ctx)
{
	ctx->intrp_data.mode = MODE_COMPILE;
}

/**
 * @brief puts value of ctx->dict.latest on the stack
 **/
void do_latest_fetch(struct forth_ctx *ctx)
{
	stack_push(ctx, (stack_cell_t)ctx->dict.latest);
}

/**
 * @brief pushes current ctx->dict.here pointer to stack
 */
void do_here(struct forth_ctx *ctx)
{
	stack_push(ctx, (stack_cell_t)ctx->dict.here);
}

/**
 * @brief Pops link pointer to word, and toggles the hidden flag.
 */
void do_hidden(struct forth_ctx *ctx)
{
	dict_header_t *w_h = (dict_header_t *)stack_pop(ctx);
	w_h -= 1;
	w_h->flags.f.hidden ^= 1;
}

/**
 * @brief toggles immediate mode of latest word
 */
void do_immediate(struct forth_ctx *ctx)
{
	ctx->dict.latest->flags.f.immediate ^= 1;
}

/**
 * @brief reads a token and pushes to stack (string and length on top)
 */
void do_word(struct forth_ctx *ctx)
{
	char token[MAX_INPUT_LEN + 1];
	char input_byte;
	stack_cell_t input_len = 0;

	do {
		input_byte = ctx->plat.getchar();

		if (input_len > (MAX_INPUT_LEN - 1)) {
			input_len = 0;
		}

		if (isspace(input_byte)) {
			if (input_len > 0) {
				memcpy(&ctx->stack[ctx->sp], token, input_len);
				stack_push_wordname(ctx, token, input_len);
				break;
			}
		}

		token[input_len++] = input_byte;
		token[input_len] = 0;
	} while (1);
}

/**
 * @brief reads an input character to top of stack
 */
void do_key(struct forth_ctx *ctx)
{
	ctx->stack[ctx->sp++] = (ctx->plat.getchar());
}

/**
 * @brief the ':' compilation word. Reads token, creates dictionary header
 * marks it hidden until ';' unhides it, switches to compilation mode to
 * begin compilation of the word. The first word (codeword) will be do_docol.
 */
void do_colon(struct forth_ctx *ctx)
{
	/* Read next word and create dictionary entry */
	do_word(ctx);
	do_create_word(ctx);

	/* Hide the word until definition is complete */
	ctx->dict.latest->flags.f.hidden = 1;

	/* Compile DOCOL as the codeword for this definition */
	word_t w = do_docol;
	compile_word(ctx, (stack_cell_t)w);

	/* Switch to compile mode */
	ctx->intrp_data.mode = MODE_COMPILE;
}

/**
 * @brief finishes compilation of currently being defined word by
 * compiling do_exit as the last word, unhides it, switches back to
 * immediate mode.
 */
void do_semicolon(struct forth_ctx *ctx)
{
	word_t w;

	/* Compile EXIT to end definition */
	w = do_exit;
	compile_word(ctx, (stack_cell_t)w);

	/* Unhide the word */
	ctx->dict.latest->flags.f.hidden = 0;

	/* Switch back to immediate mode */
	ctx->intrp_data.mode = MODE_IMMEDIATE;
}

void do_branch(struct forth_ctx *ctx)
{
	stack_cell_t offset = *(stack_cell_t *)(ctx->ip);
	/* The offset is in bytes, relative to the current IP.
	 * We convert the offset to cells and adjust IP. */
	ctx->ip += (offset / sizeof(word_t));
}

void do_0branch(struct forth_ctx *ctx)
{
	stack_cell_t offset = *(stack_cell_t *)(ctx->ip);
	stack_cell_t flag = stack_pop(ctx);

	if (flag == 0) {
		/* The offset is in bytes, relative to the current IP.
		 * We convert the offset to cells and adjust IP. */
		ctx->ip += (offset / sizeof(word_t));
	} else {
		ctx->ip = ctx->ip + 1; /* Skip the offset parameter */
	}
}

/**
 * @brief output a single character from top of stack
 */
void do_emit(struct forth_ctx *ctx)
{
	char str[2];

	str[0] = stack_pop(ctx);
	str[1] = '\0';
	ctx->plat.puts(str);
}

/**
 * @bief print the definition of a word (len, then string popped from stack)
 */
void do_see(struct forth_ctx *ctx)
{
	do_word(ctx);
	stack_cell_t len = stack_pop(ctx);
	size_t num_cells = ALIGN_UP_WORD_T(len) / sizeof(word_t);
	char *name = (char *)&ctx->stack[ctx->sp - num_cells];
	stack_sub(ctx, num_cells);

	dict_header_t *header = find_word_header(ctx, name, len);
	if (!header) {
		ctx->plat.puts("see: word not found\n");
		return;
	}

	stack_push(ctx, (stack_cell_t)header);

	print_word_def(ctx);
}

void do_wordslist(struct forth_ctx *ctx)
{
	dict_header_t *cur = ctx->dict.latest;

	while (cur != DICT_NULL) {
		stack_push(ctx, (stack_cell_t)cur);
		print_word_def(ctx);
		cur = cur->link;
	}
}

/**
 * This contains the primitive words defined in this forth.
 */

struct bultin_entry {
	char word[WORD_NAME_MAX_LEN];
	word_t c_func;
	flag_t flags;
};

struct bultin_entry builtin_table[] = {
    {.word = "docol", .c_func = do_docol, .flags = {.f.hidden = 1}},
    {.word = "lit", .c_func = do_lit, .flags = {}},
    {.word = "exit", .c_func = do_exit, .flags = {}},
    {.word = "create", .c_func = do_create_word, .flags = {}},
    {.word = ":", .c_func = do_colon, .flags = {}},
    {.word = ";", .c_func = do_semicolon, .flags = {.f.immediate = 1}},
    {.word = ",", .c_func = do_comma, .flags = {}},
    {.word = "+", .c_func = do_plus, .flags = {}},
    {.word = "-", .c_func = do_minus, .flags = {}},
    {.word = "/", .c_func = do_divide, .flags = {}},
    {.word = "*", .c_func = do_multiply, .flags = {}},
    {.word = "find", .c_func = do_find, .flags = {}},
    {.word = ".s", .c_func = do_printstack, .flags = {}},
    {.word = ".", .c_func = do_dot, .flags = {}},
    {.word = "]", .c_func = do_rbrac, .flags = {}},
    {.word = "[", .c_func = do_lbrac, .flags = {.f.immediate = 1}},
    {.word = "latest_f", .c_func = do_latest_fetch, .flags = {}},
    {.word = "here", .c_func = do_here, .flags = {}},
    {.word = "hidden", .c_func = do_hidden, .flags = {}},
    {.word = "word", .c_func = do_word, .flags = {}},
    {.word = "key", .c_func = do_key, .flags = {}},
    {.word = "drop", .c_func = do_drop, .flags = {}},
    {.word = "dup", .c_func = do_dup, .flags = {}},
    {.word = "swap", .c_func = do_swap, .flags = {}},
    {.word = "rot", .c_func = do_rot, .flags = {}},
    {.word = "over", .c_func = do_over, .flags = {}},
    {.word = "mod", .c_func = do_mod, .flags = {}},
    {.word = "1+", .c_func = do_incr, .flags = {}},
    {.word = "1-", .c_func = do_decr, .flags = {}},
    {.word = "=", .c_func = do_equal, .flags = {}},
    {.word = "<", .c_func = do_less_than, .flags = {}},
    {.word = ">", .c_func = do_greater_than, .flags = {}},
    {.word = "0=", .c_func = do_zero_equal, .flags = {}},
    {.word = "@", .c_func = do_fetch, .flags = {}},
    {.word = "!", .c_func = do_store, .flags = {}},
    {.word = "c@", .c_func = do_cfetch, .flags = {}},
    {.word = "c!", .c_func = do_cstore, .flags = {}},
    {.word = "branch", .c_func = do_branch, .flags = {}},
    {.word = "0branch", .c_func = do_0branch, .flags = {}},
    {.word = "immediate", .c_func = do_immediate, .flags = {.f.immediate = 1}},
    {.word = "2cfa", .c_func = do_2cfa, .flags = {}},
    {.word = "2dfa", .c_func = do_2dfa, .flags = {}},
    {.word = "'", .c_func = do_tick, .flags = {}},
    {.word = "emit", .c_func = do_emit, .flags = {}},
    {.word = "see", .c_func = do_see, .flags = {}},
    {.word = "words", .c_func = do_wordslist, .flags = {}},
};

int builtins_init(struct forth_ctx *ctx)
{
	word_t w;
	int len;
	dict_header_t *w_h;

	for (size_t i = 0; i < ARRAY_SIZE(builtin_table); i++) {
		/* we push a string, and the length of the string on the stack
		 */
		len = stack_push_wordname(ctx, builtin_table[i].word,
					  strlen(builtin_table[i].word));
		/* create_word consumes the length and the string to create a
		 * new dictionary entry */
		do_create_word(ctx);
		w_h = ctx->dict.latest;
		w_h->flags = builtin_table[i].flags;
		w_h->flags.f.length = len;

		/**
		 * Builtin primitive words have a single function pointer in
		 * their definition. Compiled forth words will contain what is
		 * called the codeword, i.e. do_docol function pointer as the
		 * first word. See implementation of do_colon (":" function).
		 * Additionally compiled words will contain do_exit as the last
		 * word in their definition.
		 */
		w = builtin_table[i].c_func;
		compile_word(ctx, (stack_cell_t)w);
	}

	return 0;
}

/**
 * @file builtins.c
 *
 * @brief This file contains the definitions of the built-in words
 * used by this forth.
 */
#include <string.h>
#include <stdio.h>
#include "vm.h"
#include "builtins.h"
#include "builtins_common.h"

#define ARRAY_SIZE(x)		(sizeof(x)/sizeof(x[0]))

/* forward declarations of utility functions */
int stack_push_wordname(struct forth_ctx *ctx, char *s, int len);

/* forward declarations of primitive word functions also used by other words */
void do_word(struct forth_ctx *ctx);

/**************************** primitive words functions follow ****************************/

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
	ctx->dict.latest = (dict_header_t *) ALIGN_UP_WORD_T(ctx->dict.latest);
	new = (dict_header_t *)ctx->dict.here;
	ctx->dict.here = (unsigned char *)(new + 1);

	new->link = old_latest;
	new->flags.f.hidden = 0;
	new->flags.f.immediate = 0;
	new->flags.f.length = stack_pop(ctx);

	stack_sub(ctx, ALIGN_UP_WORD_T(new->flags.f.length)/(sizeof(word_t)));
	memset(ctx->dict.here, 0, ALIGN_UP_WORD_T(new->flags.f.length));
	memcpy(ctx->dict.here, &ctx->stack[ctx->sp], new->flags.f.length);

	/**
	 * add length to 'here' round up to word_t size for alignment.
	 */
	ctx->dict.here += new->flags.f.length;
	ctx->dict.here = (unsigned char *) ALIGN_UP_WORD_T((stack_cell_t)ctx->dict.here);
	ctx->dict.latest = new;
}

void do_find(struct forth_ctx *ctx)
{
	void do_find(struct forth_ctx *ctx);
	do_word(ctx);
	stack_cell_t len = stack_pop(ctx);
	stack_sub(ctx, ALIGN_UP_WORD_T(len)/(sizeof(word_t)));
	stack_push(ctx, (stack_cell_t)find_word_header(ctx, (char *)&ctx->stack[ctx->sp], len));
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
		if (p-1 < STACK_SIZE_MAX) {
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
 * @brief removes top item from stack
 *
 * @param ctx
 */
void do_drop(struct forth_ctx *ctx)
{
	if (ctx->sp > 0) {
		(void)stack_pop(ctx);
	}
}

/**
 * @brief swaps top two items on stack
 *
 * @param ctx
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
 *
 * @param ctx
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
 *
 * @param ctx
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
 *
 * @param ctx
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
 *
 * @param ctx
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
 *
 * @param ctx
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
 *
 * @param ctx
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
 * @brief compares top two items on stack (n1 n2 -- flag), flag is true if n1 < n2.
 *
 * @param ctx
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
 * @brief compares top two items on stack (n1 n2 -- flag), flag is true if n1 > n2.
 *
 * @param ctx
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
 *
 * @param ctx
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
 *
 * @param ctx
 */
void do_fetch(struct forth_ctx *ctx)
{
	if (ctx->sp > 0) {
		stack_cell_t addr = stack_pop(ctx);
		/* Check if address is within dictionary bounds and warn if not */
		if ((void *)addr >= (void *)ctx->dict.mem &&
		    (void *)addr < (void *)(ctx->dict.mem + DICTIONARY_MEMORY_SIZE)) {
			/* Address is within dictionary */
		} else {
			ctx->plat.puts(__func__);
			ctx->plat.puts(": warning - accessing outside dictionary bounds\n");
		}
		/* Use raw address directly */
		stack_cell_t value = *(stack_cell_t *)addr;
		stack_push(ctx, value);
	} else {
		stack_push(ctx, 0);
	}
}

/**
 * @brief stores a 32-bit value to memory at given address.
 *
 * @param ctx
 */
void do_store(struct forth_ctx *ctx)
{
	if (ctx->sp >= 2) {
		stack_cell_t addr = stack_pop(ctx);
		stack_cell_t value = stack_pop(ctx);
		/* Check if address is within dictionary bounds and warn if not */
		if ((void *)addr >= (void *)ctx->dict.mem &&
		    (void *)addr < (void *)(ctx->dict.mem + DICTIONARY_MEMORY_SIZE)) {
			/* Address is within dictionary */
		} else {
			ctx->plat.puts(__func__);
			ctx->plat.puts(": warning - accessing outside dictionary bounds\n");
		}
		/* Use raw address directly */
		*(stack_cell_t *)addr = value;
	}
}

/**
 * @brief fetches a single byte from memory at given address.
 *
 * @param ctx
 */
void do_cfetch(struct forth_ctx *ctx)
{
	if (ctx->sp > 0) {
		stack_cell_t addr = stack_pop(ctx);
		/* Check if address is within dictionary bounds and warn if not */
		if ((void *)addr >= (void *)ctx->dict.mem &&
		    (void *)addr < (void *)(ctx->dict.mem + DICTIONARY_MEMORY_SIZE)) {
			/* Address is within dictionary */
		} else {
			ctx->plat.puts(__func__);
			ctx->plat.puts(": warning - accessing outside dictionary bounds\n");
		}
		/* Use raw address directly */
		unsigned char value = *(unsigned char *)addr;
		stack_push(ctx, value);
	} else {
		ctx->plat.puts(__func__);
	 	ctx->plat.puts(" stack underflow\n");
	}
}

/**
 * @brief stores a single byte to memory at given address.
 *
 * @param ctx
 */
void do_cstore(struct forth_ctx *ctx)
{
	if (ctx->sp >= 2) {
		stack_cell_t addr = stack_pop(ctx);
		stack_cell_t value = stack_pop(ctx);
		/* Check if address is within dictionary bounds and warn if not */
		if ((void *)addr >= (void *)ctx->dict.mem &&
		    (void *)addr < (void *)(ctx->dict.mem + DICTIONARY_MEMORY_SIZE)) {
			/* Address is within dictionary */
		} else {
			ctx->plat.puts(__func__);
			ctx->plat.puts(": warning - accessing outside dictionary bounds\n");
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
 *
 * @param ctx
 */
void do_2cfa(struct forth_ctx *ctx)
{
	dict_header_t *w_h = (dict_header_t *)stack_pop(ctx);
	stack_push(ctx, (stack_cell_t)(w_h + 1));
}

void do_lbrac(struct forth_ctx *ctx)
{
	ctx->intrp_data.mode = MODE_IMMEDIATE;
}

void do_rbrac(struct forth_ctx *ctx)
{
	ctx->intrp_data.mode = MODE_COMPILE;
}

/* puts value of ctx->dict.latest on the stack */
void do_latest_fetch(struct forth_ctx *ctx)
{
	stack_push(ctx, (stack_cell_t)ctx->dict.latest);
}

/**
 * @brief pushes current ctx->dict.here pointer to stack
 *
 * @param ctx
 */
void do_here(struct forth_ctx *ctx)
{
	stack_push(ctx, (stack_cell_t)ctx->dict.here);
}

/**
 * Pops link pointer to word, and toggles the hidden flag.
 */
void do_hidden(struct forth_ctx *ctx)
{
	dict_header_t *w_h = (dict_header_t *)stack_pop(ctx);
	w_h -= 1;
	w_h->flags.f.hidden ^= 1;
}

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
	stack_add(ctx, r_len/sizeof(stack_cell_t));
	stack_push(ctx, len);

	return len;
}

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

		if ((input_byte == ' ') || (input_byte == '\t')  || (input_byte == '\n') || (input_byte == '\r')) {
			if (input_len > 0) {
				memcpy(&ctx->stack[ctx->sp], token, input_len);
				stack_push_wordname(ctx, token, input_len);
				break;
			}
		}

		token[input_len++] = input_byte;
	} while(1);
}

void do_key(struct forth_ctx *ctx)
{
	ctx->stack[ctx->sp++] = (ctx->plat.getchar());
}

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

/**
 * This contains the primitive words defined in this forth.
 */

struct bultin_entry {
	char word[WORD_NAME_MAX_LEN];
	word_t c_func;
	flag_t flags;
};

struct bultin_entry builtin_table[] = {
	{.word = "exit",	.c_func = do_exit,		.flags = {}},
	{.word = "create",	.c_func = do_create_word,	.flags = {}},
	{.word = ",",		.c_func = do_comma,		.flags = {}},
	{.word = "docol",	.c_func = do_docol,		.flags = {.f.hidden = 1}},
	{.word = "+",		.c_func = do_plus,		.flags = {}},
	{.word = "-",		.c_func = do_minus,		.flags = {}},
	{.word = "/",		.c_func = do_divide,		.flags = {}},
	{.word = "*",		.c_func = do_multiply,		.flags = {}},
	{.word = "find",	.c_func = do_find,		.flags = {}},
	{.word = ".s",		.c_func = do_printstack,	.flags = {}},
	{.word = ".",		.c_func = do_dot,		.flags = {}},
	{.word = "]",		.c_func = do_rbrac,		.flags = {}},
	{.word = "[",		.c_func = do_lbrac,		.flags = {.f.immediate = 1}},
	{.word = "latest_f",	.c_func = do_latest_fetch,	.flags = {}},
	{.word = "here",	.c_func = do_here,		.flags = {}},
	{.word = "hidden",	.c_func = do_hidden,		.flags = {}},
	{.word = "word",	.c_func = do_word,		.flags = {}},
	{.word = "lit",		.c_func = do_lit,		.flags = {.f.hidden = 1}},
	{.word = "key",		.c_func = do_key,		.flags = {}},
	{.word = "drop",	.c_func = do_drop,		.flags = {}},
	{.word = "swap",	.c_func = do_swap,		.flags = {}},
	{.word = "rot",		.c_func = do_rot,		.flags = {}},
	{.word = "over",	.c_func = do_over,		.flags = {}},
	{.word = "mod",		.c_func = do_mod,		.flags = {}},
	{.word = "1+",		.c_func = do_incr,		.flags = {}},
	{.word = "1-",		.c_func = do_decr,		.flags = {}},
	{.word = "=",		.c_func = do_equal,		.flags = {}},
	{.word = "<",		.c_func = do_less_than,		.flags = {}},
	{.word = ">",		.c_func = do_greater_than,	.flags = {}},
	{.word = "0=",		.c_func = do_zero_equal,	.flags = {}},
	{.word = "@",		.c_func = do_fetch,		.flags = {}},
	{.word = "!",		.c_func = do_store,		.flags = {}},
	{.word = "C@",		.c_func = do_cfetch,		.flags = {}},
	{.word = "C!",		.c_func = do_cstore,		.flags = {}},
	{.word = ":",		.c_func = do_colon,		.flags = {}},
	{.word = ";",		.c_func = do_semicolon,		.flags = {.f.immediate = 1}},
};

int builtins_init(struct forth_ctx *ctx)
{
	word_t w;
	int len;
	dict_header_t *w_h;

	for (size_t i = 0; i < ARRAY_SIZE(builtin_table); i++) {
		/* we push a string, and the length of the string on the stack */
		len = stack_push_wordname(ctx, builtin_table[i].word, strlen(builtin_table[i].word));
		/* create_word consumes the length and the string to create a new dictionary entry */
		do_create_word(ctx);
		w_h = ctx->dict.latest;
		w_h->flags = builtin_table[i].flags;
		w_h->flags.f.length = len;

		/**
		 * Builtin primitive words have a single function pointer in their
		 * definition. Compiled forth words will contain what is called the
		 * codeword, i.e. do_docol function pointer as the first word. See
		 * implementation of do_colon (":" function). Additionally compiled
		 * words will contain do_exit as the last word in their definition.
		 */
		w = builtin_table[i].c_func;
		compile_word(ctx, (stack_cell_t)w);
	}

	return 0;
}

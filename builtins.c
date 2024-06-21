/**
 * @file builtins.c
 * @author Tavish Naruka (tavishnaruka@gmail.com)
 * @brief This file contains the definitions of the built-in words used by this
 * forth.
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "builtins.h"
#include "helper.h"
#include <string.h>
#include <stdio.h>

/* #define DBG()	printf("%s\n", __func__) */
#define DBG()

void dict_append(struct forth_ctx *ctx, void *word)
{
	DBG();
	memcpy(APPEND_PTR, word, sizeof(word_t));
	HERE += sizeof(word_t);
}

static inline void _next(struct forth_ctx *ctx)
{
	if (ctx->rsp > 0) {
		ctx->next = ctx->ip + 4;
	} else {
		ctx->next = DICT_NULL;
	}
}
#define NEXT()	_next(ctx)

/**
 * @brief pushes first the string (rounds up to 4 bytes), then the length 
 * (leaves length on top)
 * 
 * @return length of name
 */
int vm_stack_push_wordname(struct forth_ctx *ctx, char *s, int len);

/**************************** builtin words follow ****************************/

/**
 * @brief creates new dictionaly item
 * 
 * STACK_POP() -> length of the word to be created
 * STACK_SUB(ROUND_UP_4(len/4)) -> the word to be created
 */
void _do_create_word(struct forth_ctx *ctx)
{
	DBG();
	link_t old_latest = LATEST;
	
	/**
	 * 'latest' will now point to new dictionary item (at/after 'here')
	 * 
	 * (align, just in case)
	 */
	LATEST = ((HERE + sizeof(link_t) -1) / sizeof(link_t)) * sizeof(link_t);
	HERE = LATEST + sizeof(dict_header_t);

	/* this is what 'latest' is pointing to now, the new dictionary item */
	dict_header_t *new = 
		(dict_header_t *) &ctx->dict.mem[LATEST];
	new->link = old_latest;

	new->flags.hidden = 0;
	new->flags.immediate = 0;
	new->flags.length = STACK_POP();

	STACK_SUB(ROUND_UP_4(new->flags.length)/4);
	memset((char *)APPEND_PTR, 0, ROUND_UP_4(new->flags.length));
	memcpy((char *)APPEND_PTR, (char *)STACK_TOP_ADDR(), 
		new->flags.length);

	/**
	 * Now that word is appended after dictionary header, move 'here' too,
	 * and round up to 4 byte for alignment.
	 */
	HERE += new->flags.length;
	HERE = ROUND_UP_4(HERE);
}

void do_create_word(struct forth_ctx *ctx)
{
	DBG();
	_do_create_word(ctx);
	NEXT();
}

/**
 * @brief creates new dictionaly item
 * 
 * STACK_POP() -> the codeword to be appended
 * HERE += 4 after appending to current word definition
 */
void do_comma(struct forth_ctx *ctx)
{
	DBG();
	u32 codeword = STACK_POP();
	dict_append(ctx, &codeword);
	NEXT();
}

/**
 * @brief duplicates the item on top of stack
 * 
 * @param ctx 
 */
void do_dup(struct forth_ctx *ctx)
{
	DBG();
	stack_cell_t top = *(STACK_TOP_ADDR()-1);
	STACK_PUSH(top);
	NEXT();
}

void do_printstack(struct forth_ctx *ctx)
{
	DBG();
	TELL("STACK > ");
	u32 p = ctx->sp;
	char str[12];
	while (p > 0) {
		snprintf(str, sizeof(str), "%u ", ctx->stack[p-1]);
		str[sizeof(str)-1] = 0;
		TELL(str);
		p--;
	}
	TELL("\n");
	NEXT();
}

void do_dot(struct forth_ctx *ctx)
{
	DBG();
	char str[30];
	if (ctx->sp > 0) {
		snprintf(str, sizeof(str), "%u\n", STACK_POP());
		str[sizeof(str)-1] = 0;
		TELL(str);
	} else {
		snprintf(str, sizeof(str), "Data stack underflow\n");
		str[sizeof(str)-1] = 0;
		TELL(str);
	}
	NEXT();
}

/**
 * @brief pops top two items on stack and pushes result of their addition.
 * 
 * @param ctx 
 */
void do_plus(struct forth_ctx *ctx)
{
	DBG();
	stack_cell_t n1 = STACK_POP();
	stack_cell_t n2 = STACK_POP();
	STACK_PUSH(n1 + n2);
	NEXT();
}

void do_multiply(struct forth_ctx *ctx)
{
	DBG();
	stack_cell_t n1 = STACK_POP();
	stack_cell_t n2 = STACK_POP();
	STACK_PUSH(n1 * n2);
	NEXT();
}

void do_exit(struct forth_ctx *ctx)
{
	DBG();
	if (ctx->rsp > 0) {
		ctx->ip = RSTACK_POP();
		NEXT();
	} else {
		ctx->ip = DICT_NULL;
		ctx->next = DICT_NULL;
	}
}

void do_docol(struct forth_ctx *ctx)
{
	DBG();
	RSTACK_PUSH(ctx->ip);
	ctx->ip = ctx->next;
	NEXT();
}

u32 _do_find(struct forth_ctx *ctx, u32 len, char *to_find)
{
	DBG();
	char *dst_name;
	dict_header_t *p = CURRENT_WORD;
	
	while (DICT_PTR(p) != DICT_NULL) {
		dst_name = (char *)((u8 *)p + sizeof(dict_header_t));
		if ((p->flags.length == len) &&
		    (memcmp(dst_name, to_find, len) == 0) &&
		     p->flags.hidden != 1) {
			/* found an exact match */
			return DICT_PTR(p);
		}
		p = LINK_PTR(p->link);
	}

	return DICT_NULL;
}

/**
 * @brief find word in dictionary
 * STACK_POP() -> length of string
 * STACK_SUB(ROUND_UP_4(len/4))
 * ----
 * STACK_PUSH(address)
 * 
 * @param ctx 
 */
void do_find(struct forth_ctx *ctx)
{
	DBG();
	u32 len = STACK_POP();
	STACK_SUB(ROUND_UP_4(len)/4);
	char *to_find = (char *) STACK_TOP_ADDR();

	STACK_PUSH(_do_find(ctx, len, to_find));
	NEXT();
}

u32 _do_2cfa(struct forth_ctx *ctx, u32 word_ptr)
{
	DBG();
	dict_header_t *p = LINK_PTR(word_ptr);
	word_ptr += ROUND_UP_4(p->flags.length + sizeof(dict_header_t));
	return word_ptr;
}

/**
 * @brief convert pointer to word in dictionary to the code field address.
 * STACK_POP() -> dictionary word pointer
 * ----
 * STACK_PUSH(address)
 * 
 * @param ctx 
 */
void do_2cfa(struct forth_ctx *ctx)
{
	DBG();
	u32 word_ptr = STACK_POP();
	STACK_PUSH(_do_2cfa(ctx, word_ptr););
	NEXT();
}

void do_lbrac(struct forth_ctx *ctx)
{
	DBG();
	ctx->mode = MODE_IMMEDIATE;
	NEXT();
}

void do_rbrac(struct forth_ctx *ctx)
{
	DBG();
	ctx->mode = MODE_COMPILE;
	NEXT();
}

/* puts value of LATEST on the stack */
void do_latest_fetch(struct forth_ctx *ctx)
{
	DBG();
	STACK_PUSH(LATEST);
	NEXT();
}

/**
 * Pops link pointer to word, and toggles the hidden flag.
 */
void do_hidden(struct forth_ctx *ctx)
{
	DBG();
	dict_header_t *word_ptr = LINK_PTR(STACK_POP());
	word_ptr->flags.hidden ^= 1;
	NEXT();
}

void _do_word(struct forth_ctx *ctx)
{
	DBG();
	char token[MAX_INPUT_LEN + 1];
	char input_byte;
	u32 input_len = 0;

	do {
		input_byte = ctx->plat.get_key();

		if (input_len > (MAX_INPUT_LEN - 1)) {
			input_len = 0;
		}

		if ((input_byte == ' ') || (input_byte == '\t')  || (input_byte == '\n') || (input_byte == '\r')) {
			if (input_len > 0) {
				memcpy(STACK_TOP_ADDR(), token, input_len);
				vm_stack_push_wordname(ctx, token, input_len);
				break;
			}
		}

		token[input_len++] = input_byte;
	} while(1);
}

/* this pushes a word and then the length of word on to the stack */
void do_word(struct forth_ctx *ctx)
{
	DBG();
	_do_word(ctx);
	NEXT();
}

void do_lit(struct forth_ctx *ctx)
{
	DBG();
	ctx->ip += 4;
	STACK_PUSH(ctx->dict.mem[ctx->ip]);
	NEXT();
}

void do_key(struct forth_ctx *ctx)
{
	DBG();
	STACK_PUSH(ctx->plat.get_key());
	NEXT();
}

/**
 * This contains the built in words defined as C functions (not forth)
 * NOTE: update builtin_idx_e when you change this and vice versa
 */

struct bultin_entry builtin_init_c_table[] = {
	/** 
	 * NOTE: EXIT is a special one, we need this defiend first, and then
	 * we append it to each builtin c function definition
	 */
	[EXIT_IDX]		= {.word = "EXIT",	.c_func = do_exit,		.flags = {}},
	[CREATE_IDX]		= {.word = "CREATE",	.c_func = do_create_word,	.flags = {}},
	[COMMA_IDX]		= {.word = ",",		.c_func = do_comma,		.flags = {}},
	[DOCOL_IDX]		= {.word = "DOCOL",	.c_func = do_docol,		.flags = {}},
	[DUP_IDX]		= {.word = "DUP",	.c_func = do_dup,		.flags = {}},
	[PLUS_IDX]		= {.word = "+",		.c_func = do_plus,		.flags = {}},
	[MULTIPLY_IDX]		= {.word = "*",		.c_func = do_multiply,		.flags = {}},
	[FIND_IDX]		= {.word = "FIND",	.c_func = do_find,		.flags = {}},
	[TO_CFA_IDX]		= {.word = ">CFA",	.c_func = do_2cfa,		.flags = {}},
	[PRINT_STACK_IDX]	= {.word = ".s",	.c_func = do_printstack,	.flags = {}},
	[DOT_IDX]		= {.word = ".",		.c_func = do_dot,		.flags = {}},
	[RBRAC_IDX]		= {.word = "]",		.c_func = do_rbrac,		.flags = {}},
	[LBRAC_IDX]		= {.word = "[",		.c_func = do_lbrac,		.flags = {}},
	[LATEST_FETCH_IDX]	= {.word = "LATEST_F",	.c_func = do_latest_fetch,	.flags = {}},
	[HIDDEN_IDX]		= {.word = "HIDDEN",	.c_func = do_hidden,		.flags = {}},
	[WORD_IDX]		= {.word = "WORD",	.c_func = do_word,		.flags = {}},
	[LIT_IDX]		= {.word = "LIT",	.c_func = do_lit,		.flags = {}},
	[KEY_IDX]		= {.word = "KEY",	.c_func = do_key,		.flags = {}},
};


#define FORTH_LV2_WORD_MAX_LEN	(20)
/* these words are composed of builtins */
struct builtin_entry_lv2 {
	char *word;	/* word, up to WORD_NAME_MAX_LEN is used  */
	flag_t flags;
	enum builtin_idx_e c_idx_list[FORTH_LV2_WORD_MAX_LEN];
};

/* note: the problem here is that we need the correct value of the codewords
after LIT_IDX, which we cannot do statically */
struct builtin_entry_lv2 builtins_lvl2_table[] = {
	{
		.word = "DOUBLE",
		.flags = {},
		.c_idx_list = {DOCOL_IDX, DUP_IDX, PLUS_IDX, EXIT_IDX, NULL_IDX},
	},
	{
		/**
		 * We don't necessarily need to define ":" in forth, it would be
		 * easier to define as a c function, but at this stage we can do
		 * it with few simple forth words already defined.
		 */
		.word = ":",
		.flags = {},
		.c_idx_list = {DOCOL_IDX, WORD_IDX, CREATE_IDX, LIT_IDX, DOCOL_IDX,
			       COMMA_IDX, LATEST_FETCH_IDX, HIDDEN_IDX, RBRAC_IDX,
			       EXIT_IDX, NULL_IDX},
	},
	{
		.word = ";",
		.flags = {.immediate = 1},
		.c_idx_list = {DOCOL_IDX, LIT_IDX, EXIT_IDX, COMMA_IDX, LATEST_FETCH_IDX,
			       HIDDEN_IDX, LBRAC_IDX, EXIT_IDX, NULL_IDX
		},
	}
};

char builtin_entry_lv3[] = {
	": QUADRUPLE DOUBLE DOUBLE ;\n"
	": TEST DOUBLE ;\n"
	": TEST2 QUADRUPLE ;\n"
};

int builtins_init(struct forth_ctx *ctx)
{
	word_t w;
	int len;

	memset(ctx->c_func_list, 0, sizeof(ctx->c_func_list));

	for (size_t i = 0; i < ARRAY_SIZE(builtin_init_c_table); i++) {
		if (builtin_init_c_table[i].word == 0) {
			continue;
		}
		len = STACK_PUSH_WORDNAME(builtin_init_c_table[i].word);
		_do_create_word(ctx);
		CURRENT_WORD->flags = builtin_init_c_table[i].flags;
		CURRENT_WORD->flags.length = len;

		/* append the c function */
		w.is_c_func = 1;
		w.idx_or_ptr = vm_c_func_append(ctx, builtin_init_c_table[i].c_func);
		dict_append(ctx, &w.number);
		builtin_init_c_table[i].codeword = w;

		if (w.idx_or_ptr < 0) {
			TELL("WARNING: tried to load more c functions than we"
			" have space in builtin_init_c_table\n");
			break;
		}

		/* append EXIT codeword */
		w = builtin_init_c_table[EXIT_IDX].codeword;
		dict_append(ctx, &w.number);
	}

	for (size_t i = 0; i < ARRAY_SIZE(builtins_lvl2_table); i++) {
		len = STACK_PUSH_WORDNAME(builtins_lvl2_table[i].word);
		_do_create_word(ctx);
		CURRENT_WORD->flags = builtins_lvl2_table[i].flags;
		CURRENT_WORD->flags.length = len;

		for (size_t j = 0; (j < FORTH_LV2_WORD_MAX_LEN)
		     && (builtins_lvl2_table[i].c_idx_list[j] != NULL_IDX); j++) {
			/**
			 * convert builtin index to c_function (in this case it
			 * will be equivalent to the codeword address of a
			 * forth word).
			 * NOTE: in this forth >CFA and >DFA are same.
			 */
			w = builtin_init_c_table[builtins_lvl2_table[i].c_idx_list[j]].codeword;
			dict_append(ctx, &w.number);
		}
	}

	return 0;
}

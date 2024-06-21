/**
 * @file builtins.h
 * @author Tavish Naruka (tavishnaruka@gmail.com)
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef __BUILTINS_H__
#define __BUILTINS_H__

#include "vm.h"

struct bultin_entry {
	char *word;		/* word, up to WORD_NAME_MAX_LEN is used  */
	builtin_func c_func;
	flag_t flags;
	word_t codeword; 	/* code field address, set during init */
};

/**
 * This is for builtin_init_c_table, so that you can hand write 
 * builtins_lvl2_table definitions.
 */
enum builtin_idx_e {
	EXIT_IDX,
	CREATE_IDX,
	COMMA_IDX,
	DOCOL_IDX,
	DUP_IDX,
	PLUS_IDX,
	MULTIPLY_IDX,
	FIND_IDX,
	TO_CFA_IDX,
	PRINT_STACK_IDX,
	DOT_IDX,
	RBRAC_IDX,
	LBRAC_IDX,
	LATEST_FETCH_IDX,
	HIDDEN_IDX,
	WORD_IDX,
	LIT_IDX,
	KEY_IDX,
	NULL_IDX	/* use as terminator */
};

extern struct bultin_entry builtin_init_c_table[];

int builtins_init(struct forth_ctx *ctx);

#define CALL_BUILTIN(ctx, idx)	builtin_init_c_table[idx].c_func(ctx)

/* some functions also shared by the interpreter */
void _do_word(struct forth_ctx *ctx);
u32 _do_find(struct forth_ctx *ctx, u32 len, char *to_find);
u32 _do_2cfa(struct forth_ctx *ctx, u32 word_ptr);
void dict_append(struct forth_ctx *ctx, void *word);

#endif /* __BUILTINS_H__ */

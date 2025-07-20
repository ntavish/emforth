/**
 * @file builtins_common.h
 *
 * This file contains prototypes of C functions or words that
 * are shared between outer_interpreter.c and builtins.c and kept
 * here to avoid circular includes.
 */

#ifndef __BUILTINS_COMMON_H
#define __BUILTINS_COMMON_H

#include "emforth.h"
#include <string.h>

/* primitive function pointers used by outer_interpreter.c as well */
void do_docol(struct forth_ctx *ctx);
void do_exit(struct forth_ctx *ctx);
void do_lit(struct forth_ctx *ctx);

/* functions in outer_interpreter also used by primitives in builtins.c */
dict_header_t *find_word_header(struct forth_ctx *ctx, const char *name,
				size_t len);

static inline void compile_word(struct forth_ctx *ctx, stack_cell_t word_p)
{
	memcpy(ctx->dict.here, &word_p, sizeof(word_t));
	ctx->dict.here += sizeof(word_t);
}

static inline void stack_push(struct forth_ctx *ctx, stack_cell_t value)
{
	if (ctx->sp >= STACK_SIZE_MAX) {
		ctx->plat.puts("Stack overflow\n");
		ctx->sp = STACK_SIZE_MAX - 1;
	} else {
		ctx->stack[ctx->sp++] = value;
	}
}

static inline stack_cell_t stack_pop(struct forth_ctx *ctx)
{
	if (ctx->sp == 0) {
		ctx->plat.puts("Stack underflow\n");
		return 0;
	} else {
		return ctx->stack[--ctx->sp];
	}
}

static inline void stack_sub(struct forth_ctx *ctx, stack_cell_t num)
{
	if (num > ctx->sp) {
		ctx->sp = 0;
		ctx->plat.puts(__func__);
		ctx->plat.puts(" stack underflow\n");
	} else {
		ctx->sp -= num;
	}
}

static inline void stack_add(struct forth_ctx *ctx, stack_cell_t num)
{
	ctx->sp += num;
	if (ctx->sp >= STACK_SIZE_MAX) {
		ctx->plat.puts(__func__);
		ctx->plat.puts(" stack overflow\n");
		ctx->sp = STACK_SIZE_MAX - 1;
	}
}

#endif /* __BUILTINS_COMMON_H */

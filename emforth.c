/**
 * @file emforth.c
 * @author Tavish Naruka (tavishnaruka@gmail.com)
 * @brief Forth outer interpreter
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins.h"
#include "emforth.h"
#include "interpreter.h"

int emforth_init(struct forth_ctx *ctx)
{
	if (ctx == NULL || ctx->plat.puts == NULL ||
	    ctx->plat.getchar == NULL) {
		return -1;
	}

	/* initialize dictionary */
	memset(ctx->dict.mem, 0, sizeof(ctx->dict.mem));
	ctx->dict.here = &ctx->dict.mem[0];
	ctx->dict.latest = DICT_NULL;

	/* intiialize stacks */
	ctx->sp = 0;
	ctx->rsp = 0;

	/* Initialize threading registers */
	ctx->ip = NULL;
	ctx->w = NULL;

	builtins_init(ctx);

	/* Initialize interpreter */
	interpreter_init(ctx);

	ctx->plat.puts("emForth initialized\n");

	return 0;
}

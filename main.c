/**
 * @file main.c
 * @brief Main entry point for emForth sample application
 *
 * @author Tavish Naruka
 */
#include "emforth.h"
#include <stdio.h>
#include <string.h>

struct forth_ctx ctx;

int tell(const char *s)
{
	return fputs(s, stdout);
}

int main()
{
	memset(&ctx, 0, sizeof(ctx));

	/* set console functions */
	ctx.plat.puts = tell;
#ifdef __EMSCRIPTEN__
	int emforth_web_getchar();
	ctx.plat.getchar = emforth_web_getchar;
#else
	ctx.plat.getchar = getchar;
#endif

	if (emforth_init(&ctx) != 0) {
		ctx.plat.puts("Error initializing\n");
		return -1;
	}

	outer_interpreter(&ctx);

	return 0;
}

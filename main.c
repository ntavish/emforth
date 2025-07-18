/**
 * @file main.c
 * @brief Main entry point for emForth sample application
 *
 * @author Tavish Naruka
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vm.h"

int tell(const char *s)
{
	return fputs(s, stdout);
}

int main()
{
	struct forth_ctx ctx;

	memset(&ctx, 0, sizeof(ctx));

	/* set console functions */
	ctx.plat.puts = tell;
	ctx.plat.getchar = getchar;
	ctx.plat.input_file = NULL;

	if (vm_init(&ctx) != 0) {
		printf("Error initializing vm\n");
		return -1;
	}

	vm_interpreter(&ctx);

	return 0;
}

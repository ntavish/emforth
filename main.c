/**
 * @file builtins.c
 * @author Tavish Naruka (tavishnaruka@gmail.com)
 * @brief This file contains the definitions of the built-in words used by this
 * forth.
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <stdio.h>
#include <string.h>
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
	ctx.plat.tell = tell;
	ctx.plat.get_key = getchar;

	if (vm_init(&ctx) != 0) {
		printf("Error initializing vm\n");
		return -1;
	}

	vm_interpreter(&ctx);

	return 0;
}

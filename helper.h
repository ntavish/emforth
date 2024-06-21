/**
 * @file helper.h
 * @author Tavish Naruka (tavishnaruka@gmail.com)
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef __HELPER_H__
#define __HELPER_H__

#include "vm.h"
 
#define ARRAY_SIZE(x)		(sizeof(x)/sizeof(x[0]))
#define ROUND_UP_4(x)		((x+3)&~3)
/* print string console */
#define TELL(x)			ctx->plat.tell(x)

/* returns dictionary pointer (index) of last created word in dictionary */
#define LATEST			ctx->dict.latest
/* basically LATEST, but returns a raw pointer */
#define CURRENT_WORD		((dict_header_t *)&ctx->dict.mem[LATEST])
/* pointer (index) in memory to the next free byte */
#define HERE			ctx->dict.here
/* basically HERE, but gives a raw pointer */
#define APPEND_PTR		(&ctx->dict.mem[HERE])
/* raw pointer from a codefield address in the dictionary */
#define WORD_PTR(dfa)		(void *)(&ctx->dict.mem[dfa])
/* raw pointer from link address (word in dictionary) */
#define LINK_PTR(link)		(dict_header_t *)(&ctx->dict.mem[link])
/* pointer in dictionary memory from raw pointer in dictionary */
#define DICT_PTR(ptr)		(u32)((u8 *)ptr - (u8 *)&ctx->dict.mem[0])

/* data stack macros */
#define STACK_PUSH(x)		do{ctx->stack[ctx->sp]=x;ctx->sp++;}while(0)
#define STACK_POP()		ctx->stack[--ctx->sp]
#define STACK_SUB(x)		do{ctx->sp-=x;}while(0)
#define STACK_ADD(x)		do{ctx->sp+=x;}while(0)
/* be careful with this one, *STACK_TOP_ADDR will be the _next_ push position */
#define STACK_TOP_ADDR()	(stack_cell_t *)&ctx->stack[ctx->sp]

/* return stack macros */
#define RSTACK_PUSH(x)		ctx->rstack[ctx->rsp++]=x
#define RSTACK_POP()		ctx->rstack[--ctx->rsp]
#define RSTACK_PEEK()		ctx->rstack[ctx->rsp-1]

/* push a wordname to stack (up to 32 bytes, and length on top of stack) */
#define STACK_PUSH_WORDNAME(x)	vm_stack_push_wordname(ctx, x, strlen(x))

#endif /* __HELPER_H__ */

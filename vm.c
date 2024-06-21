/**
 * @file vm.c
 * @author Tavish Naruka (tavishnaruka@gmail.com)
 * @brief This is a simple stack based VM
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "vm.h"
#include "builtins.h"
#include "helper.h"
#include <bits/pthreadtypes.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

enum tok_e {
	T_NUM,
	T_WORD,
	T_NEWLINE,
};

/**
 * @brief checks if token is a valid number.
 *
 * @param tok
 * @return true token is a number
 * @return false token is not a number
 */
static inline bool vm_token_isnum(char *tok, unsigned int len)
{
	for (unsigned int i = 0; i < len; i++) {
		if (!isdigit(tok[i])) {
			return false;
		}
	}
	return true;
}

/**
 * @brief parses tokens
 *
 * @param ctx
 * @param input - input character
 * @param token - buffer to fill with token (up to MAX_INPUT_LEN bytes)
 * @return length of token if it has been read, 0 if not read yet
 */
static inline int vm_token_read(struct forth_ctx *ctx, const char input, char *token)
{
	unsigned int temp;

	/* early exit for \r, because we handle only \n */
	if (input == '\r') {
		return 0;
	}

	if (ctx->input_len > (MAX_INPUT_LEN - 1)) {
		ctx->input_len = 0;
	}

	/**
	 * XXX: change to isspace()?
	 */
	if ((input == ' ') || (input == '\t')  || (input == '\n')) {
		if (ctx->input_len > 0) {
			temp = ctx->input_len;
			memcpy(token, ctx->input_buf, temp);
			token[temp] = 0;
			ctx->input_len = 0;
			return temp;
		} else {
			return 0;
		}
	}

	ctx->input_buf[ctx->input_len++] = input;

	return 0;
}

u32 vm_c_func_append(struct forth_ctx *ctx, builtin_func func)
{
	unsigned int ret;
	if (ctx->c_func_list_idx < (MAX_C_FUNC_LIST_LEN - 1)) {
		ctx->c_func_list[ctx->c_func_list_idx] = func;
		ret = ctx->c_func_list_idx++;
		return ret;
	} else {
		return DICT_NULL;
	}
}

int vm_stack_push_wordname(struct forth_ctx *ctx, char *s, int len)
{
	/* maximum 32 len */
	len &= (WORD_NAME_MAX_LEN-1);
	/* round up to 4 byte boundary */
	u32 r_len = ROUND_UP_4(len);

	/* clear the to-be-written-to area */
	memset(&ctx->stack[ctx->sp], 0, r_len);
	/* copy to stack and adjust stack */
	memcpy(&ctx->stack[ctx->sp], s, len);
	STACK_ADD(r_len/4);
	STACK_PUSH(len);

	return len;
}

void vm_inner_interpreter(struct forth_ctx *ctx, char *tok)
{
	u32 word_ptr; 
	word_t *next_code_field;

	/* find word */
	word_ptr = _do_find(ctx, strlen(tok), tok);

	if (word_ptr == DICT_NULL) {
		/**
		 * Word not found
		 * TODO: add some error state in ctx for error handling
		 */
		TELL("ERROR: word not found (");
		TELL(tok);
		TELL(")\n");
		ctx->ip = DICT_NULL;
		return;
	}

	/* push the pointer that indicates end of return stack */
	ctx->ip = DICT_NULL;
	ctx->next = _do_2cfa(ctx, word_ptr);

	do {
		if (ctx->next == DICT_NULL) {
			break;
		}

		next_code_field = WORD_PTR(ctx->next);

		if (ctx->rsp > 0) {
			if (next_code_field->is_c_func == 1) {
				/**
				 * execute code word (is a c function)
				 * ctx->next will be incremented by the NEXT() in the c function,
				 * and/or exit is called, which returns from the current forth word (ip)
				 */
				ctx->ip = ctx->next; /* sets this as executing word */
				ctx->c_func_list[next_code_field->idx_or_ptr](ctx);
			} else {
				/* this code word is a forth word */
				ctx->next = next_code_field->idx_or_ptr; /* this will be docol, after this, rsp is > 0 */
			}
		} else {
			if (next_code_field->is_c_func == 1) {
				/**
				 * execute code word (is a c function)
				 * NOTE: this cannot be followed by another,
				 * so we just finish after this.
				 */
				ctx->ip = ctx->next; /* sets this as executing word */
				ctx->c_func_list[next_code_field->idx_or_ptr](ctx);
			} else {
				/* this code word is a forth word */
				ctx->next = next_code_field->idx_or_ptr; /* this will be docol, after this, rsp is > 0 */
			}
		}
	} while(1);
}

/* the "outer" interpreter */
static inline void process_token(struct forth_ctx *ctx, char * tok, enum tok_e tok_type)
{
	u32 word_ptr;
	void *word_ptr_raw;
	u32 word;
	dict_header_t *p;

	if (ctx->mode == MODE_IMMEDIATE) {
		switch (tok_type)
		{
		case T_NUM:
			STACK_PUSH(atoi(tok));
			break;
		case T_WORD:
			vm_inner_interpreter(ctx, tok);
			break;
		case T_NEWLINE:
		default:
			TELL("OK\n");
			break;
		}
	} else {
		/* MODE_COMPILE */
		switch (tok_type)
		{
		case T_NUM:
			word_ptr = _do_find(ctx, strlen("LIT"), "LIT");
			word_ptr_raw = WORD_PTR(_do_2cfa(ctx, word_ptr));
			dict_append(ctx, word_ptr_raw);

			word = atoi(tok);
			dict_append(ctx, &word);
			break;
		case T_WORD:
			word_ptr = _do_find(ctx, strlen(tok), tok);
			if (word_ptr == DICT_NULL) {
				TELL("ERROR: word not found (");
				TELL(tok);
				TELL(")\n");
				ctx->ip = DICT_NULL;
				ctx->next = DICT_NULL;
				return;	
			}
			p = LINK_PTR(word_ptr);
			if (p->flags.immediate) {
				vm_inner_interpreter(ctx,tok);
			} else {
				word_ptr_raw = WORD_PTR(_do_2cfa(ctx, word_ptr));
				dict_append(ctx, word_ptr_raw);
			}
			break;
		case T_NEWLINE:
		default:
			TELL("OK\n");
			break;
		}
	}
}

int vm_interpreter(struct forth_ctx *ctx)
{
	char token[MAX_INPUT_LEN + 1];
	char input_byte;
	int rc;

	do {
		input_byte = ctx->plat.get_key();

		rc = vm_token_read(ctx, input_byte, token);
		if (rc > 0) {
			if (vm_token_isnum(token, rc)) {
				process_token(ctx, token, T_NUM);
			} else {
				process_token(ctx, token, T_WORD);
			}
		}
		if (input_byte == '\n') {
			process_token(ctx, token, T_NEWLINE);
		}
	} while(1);
}

int vm_init(struct forth_ctx *ctx)
{
	if (ctx == NULL || ctx->plat.tell == NULL ||
	    ctx->plat.get_key == NULL) {
		return -1;
	}

	memset(ctx->dict.mem, 0, sizeof(ctx->dict.mem));
	ctx->dict.latest = DICT_NULL;
	ctx->dict.here = 0;
	ctx->c_func_list_idx = 0;

	ctx->sp = 0;
	ctx->rsp = 0;

	ctx->ip = DICT_NULL;

	ctx->mode = MODE_IMMEDIATE;

	builtins_init(ctx);

	TELL("VM initialized\n");

	return 0;
}

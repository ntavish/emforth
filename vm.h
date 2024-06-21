/**
 * @file vm.h
 * @author Tavish Naruka (tavishnaruka@gmail.com)
 * @brief This is a simple stack based VM
 * @version 0.1
 * @date 2021-03-28
 *
 * @copyright Copyright (c) 2021
 *
 */

#ifndef __FORTH_VM_HEADER__
#define __FORTH_VM_HEADER__

#include "types.h"

/**
 * Some parameters which can be tuned.
 */

#define STACK_SIZE_MAX		(40u) /* unit is cells */
#define RSTACK_SIZE_MAX		(40u) /* unit is cells */
#define DICTIONARY_MEMORY_SIZE	(2048u)

/**
 * link_t is type used to point to items on stack or in dictionary.
 * (Not for codewords)
 */
typedef u32 link_t;

/* this must fit the 'length' field in the flag_t struct'*/
#define WORD_NAME_MAX_LEN	(32u) /* 2^5 */

/**
 * NOTE: when pushed to the return stack, this is essentially the return pointer
 * to the interpreter.
 */
#define DICT_NULL		((u32)0xFFFFFFFF)

/* see */
#define MAX_C_FUNC_LIST_LEN	(200u)

/* interpreter */
#define MAX_INPUT_LEN 		(WORD_NAME_MAX_LEN)

/**
 * Dictionary:
 *
 * The latest defined word is pointed to by a variable latest.
 *
 * Header:
 * 
 *  ------------------ 
 *  |                |
 *  |    4 bytes     |
 *  |  Link pointer  | ---> pointer to previous item (or DICT_NULL if first)
 *  |                |
 *  |----------------|
 *  |                |
 *  |   1 byte flags |
 *  |                |
 *  |  1 immediate   | --> interptr runs this instruction even in compile mode
 *  |  1 hidden      | --> hidden is used to ignore definition in search
 *  |  1 spare       |
 *  |  5 length      | --> names are at most 32 characters long
 *  |                |
 *  |----------------|
 *  |                |
 *  | Length bytes   | --> word name, length is in length field above
 *  | of name        |
 *  | (any padding)  |
 *  |----------------|
 *  |                |
 *  |  (definition)  | --> sequence of codeword pointers (codefield or datafield)
 *  |                |
 *  ------------------
 * 
 * NOTE: start of header and start of definition are 4 byte aligned.
 * 
 * NOTE: words here can be of two types, which are described in word_t 
 * type definition. If it is a forth word, it starts with docol, and ends with
 * exit. If it is a c function, it contains the NEXT() macro as last line.
 */

typedef union {
	struct {
		u8 immediate : 1;
		u8 hidden : 1;
		u8 spare : 1;
		u8 length : 5;
	};
	u8 flag_byte;
} flag_t;

/**
 * The dictionary header consists of the following two fields in sequence.
 * Without packing pragma, we may get structure padding.
 */

typedef struct __attribute__((packed)) {
	link_t link;
	flag_t flags;
} dict_header_t;

/**
 * (definition) part above consists of one or multiple codewords.
 * - is_c_func - if 1 -> then this is a C function in the builtins_table
 *               if 0 -> then this is a word in the dictionary
 *                
 * - idx_or_ptr - if is_c_func=1 -> index into ctx->c_func_list
 *                if is_c_func=0 -> points to another definition (codefield,
 *                                  not header)
 * 
 * In Jonesforth, you have 'codeword' which is the 'docol' word, in our case
 * we will use is_c_func to judge whether the equivalent of docol needs to be
 * done. We need to use a index into a table of pointers because we want this
 * word_t to be a fixed size (32-bit), while not relying on some compiler flag
 * to force 32-bit pointers. 
 */
typedef  union {
	struct {
		u32 is_c_func : 1;
		u32 idx_or_ptr : 31;
	};
	u32 number;
} word_t;

/**
 * Just for easier understanding (and maybe some other things), we separate
 * the dictionary state definition from struct forth_ctx.
 */
typedef struct
{
	u8 mem[DICTIONARY_MEMORY_SIZE];

	/* points to latest defined word in dictionary */
	link_t latest;

	/* points to next free byte */
	link_t here;
} dict_t;

/**
 * Stacks:
 * We have two stacks:
 * 1) the interpreter stack, this is the data stack, for parameters
 * 2) the return stack (rstack in struct forth_ctx)
 */

typedef u32 stack_cell_t;

typedef enum {
	MODE_IMMEDIATE = 0,
	MODE_COMPILE = 1
} vm_mode_e;

typedef enum {
	BASE_10 = 0,
	BASE_16 = 1
} vm_base_e;

struct platform_s {
	int (*tell)(const char *); /* print string */
	int (*get_key)(); /* get input key */
};

/**
 * Forth machine state:
 * We are implementing the underlying machine on which our forth runs in C
 * so as not to rely on architechture or implementation specific assembly
 * code. All of our state, stacks, latest, etc. will be contained in the
 * following struct.
 * Forward declaration to use in the builtin_func type definition which has
 * this as argument.
 */
struct forth_ctx;

/**
 * We need this definition here instead of in builtins.h because forth_ctx uses
 * it.
*/
typedef void (*builtin_func)(struct forth_ctx *ctx);

struct forth_ctx {
	/* dictionary related state */
	dict_t dict;

	/**
	 * we use this table to store the pointers to the C functions
	 * corresponding to words defined as C functions.
	 * This list will also act as a lookup from 'pointer' (an index in this
	 * case since this is a jump table) to dictionary word.
	 */
	builtin_func c_func_list[MAX_C_FUNC_LIST_LEN];
	unsigned int c_func_list_idx;

	/* stacks */
	stack_cell_t stack[STACK_SIZE_MAX];
	stack_cell_t rstack[RSTACK_SIZE_MAX];
	i32 sp; /* stack pointer - current insert position */
	i32 rsp; /* return stack pointer - current insert position*/
	
	/**
	 * Index into the dictionary
	 * When we execute forth code
	*/
	u32 ip;
	/**
	 * We use this in the outer interpreter to find the next one
	*/
	u32 next;

	/* interpreter */
	vm_mode_e mode; /* interpreter or compiler mode*/
	vm_base_e base; /* decimal or hexadecimal*/
	char input_buf[MAX_INPUT_LEN + 1];
	unsigned int input_len;

	/* platform specific functions */
	struct platform_s plat;
};

/**
 * virtual machine functions
 */

/**
 * @brief Append to c_func_list and return index.
 * 
 * @return positive index into ctx->c_func_list on success.
 * DICT_NULL when there is no more space.
 */
u32 vm_c_func_append(struct forth_ctx *ctx, builtin_func func);

/**
 * @brief Initialize state.
 * @param ctx valid pointer to struct forth_ctx.
 * @returns 0 on success
 */
int vm_init(struct forth_ctx *ctx);

/**
 * @brief The interpreter loop
 * 
 * @param ctx that has been initialized with vm_init
 * @return int 
 */
int vm_interpreter(struct forth_ctx *ctx);

#endif /* __FORTH_VM_HEADER__ */

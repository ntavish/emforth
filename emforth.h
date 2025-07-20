/**
 * @file emforth.h
 * @author Tavish Naruka (tavishnaruka@gmail.com)
 * @brief A direct threaded code forth desgined to be embeddable.
 *
 * @copyright Copyright (c) 2025
 */

#ifndef __FORTH_EMFORTH_HEADER__
#define __FORTH_EMFORTH_HEADER__

#include <stdbool.h>
#include <stdint.h>

/**
 * Dictionary:
 *
 * The latest defined word is pointed to by a variable latest.
 *
 * Header:
 *
 *  ------------------
 *  |                |
 *  |  type:         |
 *  |  word_t        |
 *  |                |
 *  |  Link pointer  | ---> pointer to previous item (or DICT_NULL if first)
 *  |                |
 *  |----------------|
 *  |                |
 *  |  type:         |
 *  |  flag_t        |
 *  |                |
 *  |  1b immediate  | --> interpretr executes this word even if it is in compile mode
 *  |  1b hidden     | --> hidden is used to ignore definition in search
 *  |  1b spare      |
 *  |  5b length     | --> names are at most 31 characters long
 *  |                |
 *  |----------------|
 *  |                |
 *  | type:          |
 *  | char array     |
 *  |                |
 *  | Length bytes   | --> word name, length is in length field above
 *  | of name        |
 *  | (any padding)  |
 *  |----------------|
 *  |                |
 *  |  type:         | --> first word is called codeword, for primitive words
 *  |  word_t        |     it is simply one pointer (word_t), for high level
 *  |  (one or more) |     words it is always &docol(), followed by more words
 *  |                |     and ending with an &exit.
 *  |   Definition   |
 *  |                |
 *  ------------------
 *
 * NOTE: start of header and start of definition are sizeof(word_t) aligned.
 *
 * NOTE: words here can be of two types, which are described in word_t
 * type definition. If it is a forth word, it starts with docol, and ends with
 * exit. If it is a c function, it contains the NEXT() macro as last line.
 */

/**
 * Forth machine state:
 * We are implementing the underlying machine on which our forth runs in C
 * so as not to rely on architechture or implementation specific assembly
 * code. All of our state, stacks, latest, etc. will be contained in the
 * following struct.
 *
 * Forward declaration because builtin_func type definition which has
 * this as argument.
 */
struct forth_ctx;

/**
 * Words are simply function pointers.
 */
typedef void (*word_t)(struct forth_ctx *ctx);

typedef union {
	struct {
		unsigned char immediate : 1;
		unsigned char hidden : 1;
		unsigned char spare : 1;
		unsigned char length : 5;
	} f;
	unsigned char flag_fields;
} flag_t;

/* this must fit the 'length' field in the flag_t struct'*/
#define WORD_NAME_MAX_LEN ((1 << 5) - 1)

/*
 * We do not need this to be packed, as we always access the dictionary
 * definition through a pointer.
 */
typedef struct dict_header_s {
	struct dict_header_s *link;
	flag_t flags;
} dict_header_t;

#define DICT_NULL ((dict_header_t *)NULL)

/*
 * Dictionary definition words are sizeof(word_t) aligned as
 * mentioned above. Use the following utility macro.
 */
#define ALIGN_UP_WORD_T(x)                                                     \
	(((stack_cell_t)(x) + (sizeof(word_t) - 1)) & ~(sizeof(word_t) - 1))

/**
 * Stacks:
 * We have two stacks:
 * 1) the interpreter stack, this is the data stack, for parameters
 * 2) the return stack (rstack in struct forth_ctx)
 *
 * Stacks sizes and dictionary sizes in word_t units
 */
#define STACK_SIZE_MAX (stack_cell_t)(1024u)
#define RSTACK_SIZE_MAX (stack_cell_t)(1024u)
#define DICTIONARY_MEMORY_SIZE (stack_cell_t)(8192u)
typedef intptr_t stack_cell_t;

typedef struct {
	unsigned char mem[DICTIONARY_MEMORY_SIZE];

	/* points to latest defined word header in dictionary */
	dict_header_t *latest;

	/* points to next free byte in the dictionary */
	unsigned char *here;
} dict_t;

/* platform dependent interface that application must provide */
struct platform_s {
	int (*puts)(const char *); /* print string */
	int (*getchar)();	   /* get input key */
};

/* == interpreter related things == */
enum tok_e {
	T_NUM,
	T_WORD,
	T_NEWLINE,
};

typedef enum { MODE_IMMEDIATE = 0, MODE_COMPILE = 1 } mode_e;

#define MAX_INPUT_LEN (WORD_NAME_MAX_LEN * 10)

struct interpreter_data {
	mode_e mode;	 /* interpreter or compiler mode*/
	bool in_comment; /* true when processing backslash comment until newline
			  */
};

struct forth_ctx {
	/* dictionary related state */
	dict_t dict;

	/* stacks */
	stack_cell_t stack[STACK_SIZE_MAX];
	word_t *rstack[RSTACK_SIZE_MAX];
	stack_cell_t sp;  /* stack pointer - current insert position */
	stack_cell_t rsp; /* return stack pointer - current insert position*/

	word_t *ip; /* this is pointing to a word_t in the definition */
	word_t *w;  /* current word being executed */

	/* interpreter data */
	struct interpreter_data intrp_data;

	/* platform specific data */
	struct platform_s plat;
};

/**
 * @brief Initialize state.
 * @param ctx valid pointer to struct forth_ctx.
 * @returns 0 on success
 */
int emforth_init(struct forth_ctx *ctx);

/**
 * @brief The interpreter loop
 *
 * @param ctx that has been initialized with emforth_init
 * @return int
 * Does not return until error or encountering EOF.
 */
int outer_interpreter(struct forth_ctx *ctx);

#endif /* __FORTH_EMFORTH_HEADER__ */

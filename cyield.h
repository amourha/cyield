#ifndef CYIELD_H
#define CYIELD_H

#include <stdbool.h>
#include <stdint.h>
#include <setjmp.h>

#define PP_NARG(...) \
         PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
         PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N(\
          _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
         _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
         _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
         _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
         _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
         _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
         _61, _62, _63, N, ...) N
#define PP_RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

#define STACK_TOP(coro) (uintptr_t)(coro->stack + sizeof(coro->stack))

#define offsetof(type, element) ((size_t)&(((type *)0)->element))
#define countof(array) (sizeof(array) / sizeof(array[0]))

#define CHECK(cond)                     \
    if(!(cond)) {                       \
        printf("Error: " #cond "\n");   \
        goto error;                     \
    }

#define MAX_COROUTINES (64)
#define COROUTINE_STACK_SIZE (4096)
#define MAGIC (0xcb4b88d6d61c81d1)

struct reg_args {
    uintptr_t rdi;
    uintptr_t rsi;
    uintptr_t rdx;
    uintptr_t rcx;
    uintptr_t r8;
    uintptr_t r9;
};

typedef struct coroutine {
    unsigned char stack[COROUTINE_STACK_SIZE];
    struct reg_args rargs;
    uintptr_t magic;

    jmp_buf callee_ctx;
    jmp_buf caller_ctx;

    bool is_done;
    bool is_started;
    void *func;
    unsigned long yield_val;
} coroutine_t;

bool next(coroutine_t *c, unsigned long *value);

void yield(unsigned long value);

coroutine_t *init_generator(void *func, unsigned int nargs, ...);

#define generator(func, ...) \
    init_generator(func, PP_NARG(__VA_ARGS__),__VA_ARGS__);

#endif // CYIELD_H

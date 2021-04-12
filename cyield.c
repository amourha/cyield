#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include "cyield.h"

static coroutine_t *g_coroutines[MAX_COROUTINES] = {NULL};

static bool save_coroutine(coroutine_t *c) {
    assert(c);

    for (unsigned int idx = 0; idx < countof(g_coroutines); ++idx) {
        if (!g_coroutines[idx]) {
            g_coroutines[idx] = c;
            return true;
        }
    }
    return false;
}

static void free_coroutine(coroutine_t *c) {
    assert(c);

    for (unsigned int idx = 0; idx < countof(g_coroutines); ++idx) {
        if (g_coroutines[idx] == c) {
            free(c);
            g_coroutines[idx] = NULL;
        }
    }
}

__attribute__((naked))
static void trampoline(coroutine_t *c) {
    // Switch stack to coroutine stack
    asm("leaq %c0(%%rdi), %%r11"::"i"(offsetof(struct coroutine, stack)));
    asm("leaq %c0(%%r11), %%rsp"::"i"(sizeof((struct coroutine *) 0)->stack - sizeof(void *)));

    // Trampolined function
    asm("movq %c0(%%rdi), %%rax"::"i"(offsetof(struct coroutine, func)));

    // Set function parameters
    asm("movq %c0(%%rdi), %%rsi"::"i"(offsetof(struct coroutine, rargs.rsi)));
    asm("movq %c0(%%rdi), %%rdx"::"i"(offsetof(struct coroutine, rargs.rdx)));
    asm("movq %c0(%%rdi), %%rcx"::"i"(offsetof(struct coroutine, rargs.rcx)));
    asm("movq %c0(%%rdi), %%r8"::"i"(offsetof(struct coroutine, rargs.r8)));
    asm("movq %c0(%%rdi), %%r9"::"i"(offsetof(struct coroutine, rargs.r9)));
    asm("movq %c0(%%rdi), %%rdi"::"i"(offsetof(struct coroutine, rargs.rdi)));

    // Jump to function
    asm("jmp *%rax");
}

static coroutine_t *find_coroutine(void) {
    // Try to look for which coroutine our current stack belongs to.
    uintptr_t sp;
    asm volatile("movq %%rsp, %0" : "=r"(sp));

    for (unsigned int idx = 0; idx < countof(g_coroutines); ++idx) {
        if (!g_coroutines[idx]) {
            continue;
        }
        if (sp >= (uintptr_t) g_coroutines[idx]->stack && sp <= STACK_TOP(g_coroutines[idx])) {
            return g_coroutines[idx];
        }
    }
    return NULL;
}

void yield(unsigned long value) {
    coroutine_t *c = (coroutine_t *) find_coroutine();
    // This could happen when there is no more space left on the stack.
    // In that case we should just exit.
    // TODO: Enlarge the stack when the space is "over".
    CHECK(c);

    assert(MAGIC == c->magic);

    c->yield_val = value;
    if (!setjmp(c->callee_ctx)) {
        longjmp(c->caller_ctx, 1);
    }
    return;

error:
    exit(-1);
}

bool next(coroutine_t *c, unsigned long *value) {
    assert(value);
    assert(c);

    if (!setjmp(c->caller_ctx)) {
        if (!c->is_started) {
            c->is_started = true;
            trampoline(c);
        } else {
            longjmp(c->callee_ctx, 1);
        }
    }

    if (c->is_done) {
        free_coroutine(c);
        return false;
    }

    *value = c->yield_val;
    return true;
}

static void stop_iteration(void) {
    coroutine_t *c = (coroutine_t *) find_coroutine();
    CHECK(c);

    assert(MAGIC == c->magic);
    assert(false == c->is_done);

    printf("stopping iteration\n");
    c->is_done = true;
    longjmp(c->caller_ctx, 1);

error:
    exit(-1);
}

#define MAX_ARGS (6)

coroutine_t *init_generator(void *func, unsigned int nargs, ...) {
    assert(func);
    assert(nargs <= MAX_ARGS);

    coroutine_t *c = malloc(sizeof(coroutine_t));
    CHECK(c);

    CHECK(save_coroutine(c));

    c->is_done = false;
    c->is_started = false;
    c->func = func;
    c->magic = MAGIC;

    va_list ap;
    va_start(ap, nargs);

    // Save all registers
    if (nargs > 0)
        c->rargs.rdi = va_arg(ap, uintptr_t);
    if (nargs > 1)
        c->rargs.rsi = va_arg(ap, uintptr_t);
    if (nargs > 2)
        c->rargs.rdx = va_arg(ap, uintptr_t);
    if (nargs > 3)
        c->rargs.rcx = va_arg(ap, uintptr_t);
    if (nargs > 4)
        c->rargs.r8 = va_arg(ap, uintptr_t);
    if (nargs > 5)
        c->rargs.r9 = va_arg(ap, uintptr_t);

    // TODO: Support more variables on the stack

    *(((uintptr_t *) STACK_TOP(c)) - 1) = (uintptr_t) stop_iteration;

    va_end(ap);

    return c;

error:
    return NULL;
}

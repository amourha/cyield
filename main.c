#include "cyield.h"
#include <stdio.h>

void test2(int a) {
    printf("%s: %d\n", __FUNCTION__ , a);
    yield(5);
}

void test(int a, int b, int c, int d) {
    yield(a);
    test2(1);
    yield(b);
    test2(2);
    yield(c);
    yield(d);
}

int main() {
    coroutine_t *c = generator(test, 1, 2, 3, 4);
    CHECK(c);

    unsigned long val;
    while (next(c, &val)) {
        printf("yield val: %lu\n", val);
    }
    return 0;
error:
    return -1;
}

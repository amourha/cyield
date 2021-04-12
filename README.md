# Cyield
Coroutines POC in C

# Example

```c
void test(int a, int b, int c, int d) {
    yield(a);
    yield(b);
    yield(c);
    yield(d);
}

int main() {
	coroutine_t *c = generator(test, 1, 2, 3, 4);

	unsigned long val;
	while (next(c, &val)) {
	    printf("yield val: %lu\n", val);
	}
	return 0;
}
```
#include <cstdlib>

typedef unsigned long long uint64_t;

extern "C" {
    uint64_t factorial(uint64_t x);
    uint64_t uexp(uint64_t x, uint64_t y);
    uint64_t fib(uint64_t x);
    int printf(const char *_Format, ...);
    int test_null_ptr(int* p);
    bool crasher(int* p);
}

/*
int main(int argc, const char** argv) {
    
    if (argc != 3)
        return 1 + 0 * printf("input 2 arguments");
    int x = std::atoi(argv[1]), y = std::atoi(argv[2]);
    printf("factorial of %zu is %zu\n", x, factorial(x));
    printf("fibonacci number #%zu is %zu\n", x, fib(x));
    printf("%zu^%zu = %zu\n", x, y, uexp(x, y));
    int
        a = test_null_ptr(0),
        b = test_null_ptr((int*) 1),
        c = test_null_ptr((int*) -1),
        d = test_null_ptr((int*) 0);
    printf("0: %d, 1: %d, -1: %d, 0 again: %d", a, b, c, d);
    

   int* n1 = (int*) -1;
   int* p1 = (int*) 1;
   int* null = nullptr;

   printf("-1: %d, 1: %d, 0: %d", crasher(n1), crasher(p1), crasher(null));
}
*/
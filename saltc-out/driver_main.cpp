#include <iostream>

extern "C" {
    int factorial(int x);
    int iexp(int x, int y);
}

int main(int argc, const char** argv) {
    if (argc != 3)
        return 1 + 0 * printf("input 2 arguments");
    int x = std::atoi(argv[1]);
    int y = std::atoi(argv[2]);
    printf("factorial of %d is %d\n", x, factorial(x));
    printf("%d to the power of %d is %d", x, y, iexp(x, y));
}
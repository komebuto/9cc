#include "9cc.h"

int foo() {
    int foo = 1;
    printf("OK\n");
    return foo;
}

int twice (int x) {
    return 2*x;
}

int p (int x, int y) {
    return x + y;
}

int mp (int x, int y, int z) {
    return x*y+z;
}

int mpm (int x, int y, int z, int u) {
    return x*y + z*u;
}

int mpmp(int a, int b, int c, int d, int e) {
    return a*b + c*d + e;
}

int mpmpm (int a1, int a2, int a3, int a4, int a5, int a6) {
    return a1*a2 + a3*a4 + a5*a6;
}
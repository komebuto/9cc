#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    cc -static -o tmp tmp.s function.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
	    echo "$input => $actual"
    else
	    echo "$input => $expected expected, but got $actual"
	    exit 1
    fi
}
#assert 5 'int main() { int x=3; int y=5; return *(&x+1); }'
#assert 3 'int main() { int x=3; int y=5; return *(&y-1); }'
#assert 7 'int main() { int x=3; int y=5; *(&x+1)=7; return y; }'
#assert 7 'int main() { int x=3; int y=5; *(&y-1)=7; return x; }'
#assert 2 'int main() { int x=3; return (&x+2)-&x; }'

assert 8 'int main() { int x=3, y=5; return x+y; }'
#assert 5 'int main() { int x=3; int y=5; return *(&x+1); }'

assert 9 "int x[1][2]; int main() { **x=9; **(x+1)=10; return **x; }"
assert 8 'int main() { int x, y; x=3; y=5; return x+y; }'
#assert 8 'int main() { int x=3, y=5; return x+y; }'
assert 3 'int pd(int x, int y) { return x+y; } int main() { int x=1; int y=2; return pd(x,y); }'
assert 6 "int x, y, z; int main() { x=1; y=2; z=3; return x+y+z; }"
assert 2 "int x=2; int main() { return x; }"
assert 5 "int x=2, y=3; int main() { return x+y; }"
assert 6 "int x=4; char y=2; int main() { return x+y; }"

#assert 3 'int main() { int *x[2]={1,2}; return x[0]+x[1]; }'
assert 1 "int main() {int x=1; return x;}"
assert 97 "int main() { return \"abc\"[0]; }"
assert 98 "int main() { return \"abc\"[1]; }"
assert 99 "int main() { return \"abc\"[2]; }"
assert 0 "int main() { return \"abc\"[3]; }"
assert 97 "int main() { char *x; x=\"abc\"; return x[0]; }"
assert 97 "int main() { char *x=\"abc\"; return x[0]; }"

assert 0 "int main() { char *x; x=\"abc\"; print(x); return 0;}"

assert 2 "int main() { char x[2]; return sizeof(x); }"
assert 1 "int main() { char x; x = 1; return x; }"
assert 2 "int main() { char x[2]; x[1] = 2; return x[1]; }"
assert 3 "int main() { char x[2]; x[0] = 1; x[1] = 2; return x[0] + x[1]; }"
assert 6 "int main() { char x[2]; char y; int z; x[0] = 1; x[1] = -1; y = 10; z = -4; return x[0]+x[1]+y+z; }"
assert 1 "int main() { char x; char y[2]; int z; z = 1; return z; }"
assert 10 "int x[1][2]; int main() { *(*x+1) = 10; x[0][0] = 0; return *(*x+1); }"
assert 2 "int x[1][2]; int main() { x[0][1] = 2; x[0][0] = 0; return x[0][1]; }"
assert 3 "int x[2]; int main() { x[0] = 1; x[1] = 2; return x[0] + x[1]; }"
assert 3 "int x; int main() { int y; x = 1; y = 2; return x + y; }"
assert 1 "int x; int main() { x = 1; return x; }"
assert 1 "int main() { int x; x = 1; return x; } int x;"
assert 2 "int main() { int a[2]; 1[a] = 5; a[0] = 2; return 0[a]; }"
assert 10 "int main() { int a[2][3]; a[0][1] = 10; return a[0][1]; }"
assert 10 "int main() { int a[2][3]; **a = 10; return a[0][0]; }"
assert 10 "int main() { int a[3]; *a = 2; *(a+1) = 10; a[2] = 5; return *(a-1+2); }"
assert 10 "int main() { int a[3]; *a = 2; *(a+1) = 10; a[2] = 5; return 1[a]; }"
assert 5 "int main() { int a[3]; *a = 2; *(a+1) = 10; a[2-1] = 5; return a[2/2]; }"
assert 5 "int main() { int a[3]; *a = 2; *(a+1) = 10; a[2-1] = 5; return *(a+1); }"
assert 2 "int main() { int a[3]; *a = 2; *(a+1) = 10; a[2-1] = 5; return *(a); }"
assert 12 "int main() { int a[3]; return sizeof(a); }"
assert 16 "int main() { int a[2][2]; return sizeof(a); }"
assert 8 "int main() { int a[2][2]; return sizeof(*a); }"
assert 3 "int main () { int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p + *(p + 1); }"
assert 1 "int main() { int a[10]; int b; b = 1; return b; }"
assert 1 "int main() { int *p; int *q; int x; int y; x = 0; y = 1; p = &x; q = &y; return *p + *q; }"
assert 4 "int main() { int x; return sizeof(x); }"
assert 4 "int main() { int x; return sizeof(x+1); }"
assert 8 "int main() { int *p; return sizeof(p); }"
assert 8 "int main() { int *p; return sizeof(p+1); }"
assert 4 "int main() { int *p; return sizeof(*p); }"
assert 4 "int main() { return sizeof(sizeof(1)); }"
assert 8 "int main () { int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 3; return *q; }"
assert 3 "int main () { int x; int *y; y = &x; *y = 3; return x; }"
assert 1 "int main () { int x; x = 1; return x; }"
assert 1 "int main() { int x; int *y; x = 1; y = &x; return *y; }"
assert 3 "int main() { int x; int y; x = 3; y = 3; return x; }"
assert 3 "int main() { int x; int y; int z; x = 1; y = 2; z = plus(x, y); return z; } int plus(int x,int y){ return x+y; }"
assert 0 "int main() { return 0; }"
assert 1 "int main () { 1 == 1; }"
assert 0 "int main () { 1 != 1; }"
assert 1 "int main () { 2 > 1; }"
assert 1 "int main() { 2 >= 2; }"
assert 0 "int main(){ 2 < 1; }"
assert 0 "int main(){ 2 <= 1; }"
assert 5 "int main(){ int a1; int b_; a1 = -12 + (2 - 2 * 3) / 2 + 17; b_ = + 2; return a1 + b_; }"
assert 1 "int main(){ int return1; return1 = 1; return return1; }"
assert 1 "int main(){ int a; a = 1; if (a == 0) return 2; else return 1; }"
assert 2 "int main(){ int a; a = 1; if (a == 1) return 2; else return 1; }"
assert 10 "int main() { int n; int a; n = 0; for (a=0; a<10; a=a+1) n = n + 1; return n; }"
assert 0 "int main(){ int n; int a; n = 0; for (a=0; a<0; a=a+1) n = n + 1; return n; }"
assert 1 "int main(){ int n; n = 1; while(0) return n+1; return n; }"
assert 2 "int main(){ int n; n = 1; while(1) return n+1; return n; }"
assert 5 "int main(){ int n; n = 1; while (n <= 10) { if (n == 5) { return n; } n = n+1; } return n; }"
assert 10 "int main(){ int n; n = 1; while (n < 10) { if (n == 20) { return n; } n = n+1; } return n; }"
assert 1 "int main(){ int foo; foo = 1; foo(); return foo; }"
assert 1 "int main(){ int foo; foo = 0; foo = foo(); return foo; }"
assert 1 "int main () {return 1;}"
assert 44 "int main(){ int a;int b;int c;int d;int e;int f;int g; a=1;b=2;c=3;d=4;e=5;f=6; g=mpmpm(a,b,c,d,e,f); return g; }"
assert 11 "int main(){ int a;int b;int c;int d;int e;int f; a=1; b=2; c=3; d=2; e=3; f = mpmp(a,b,c,d,e); return f; }"
assert 14 "int main(){ int x;int y;int z;int u;int w; x = 1; y = 2; z = 3; u = 4; w = mpm(x, y, z, u); return w; }"
assert 14 "int main(){ int x;int y;int z; x = 1; y = 2; z = 3; return mpm(x, y, z, 4); }"
assert 5 "int main(){ int a; int b; int c; int d; a = 1; b=2; c=3; d = mp(a, b, c); return d; }"
assert 5 "int main(){int u;return u = mp(1, 2, 3); }"
assert 3 "int main(){ int x; int y; int z; x = 1; y = 2; z = p(x, y); return z; }"
assert 2 "int main(){ int x; x = 1; return twice(x); }"
assert 1 "int main() { char x; char y[2]; int z; z = 1; return z; }"

assert 0 'int main() { return 0; }'
assert 42 'int main() { return 42; }'
assert 21 'int main() { return 5+20-4; }'
assert 41 'int main() { return  12 + 34 - 5 ; }'
assert 47 'int main() { return 5+6*7; }'
assert 15 'int main() { return 5*(9-6); }'
assert 4 'int main() { return (3+5)/2; }'
assert 10 'int main() { return -10+20; }'
#assert 10 'int main() { return - -10; }'
#assert 10 'int main() { return - - +10; }'

assert 0 'int main() { return 0==1; }'
assert 1 'int main() { return 42==42; }'
assert 1 'int main() { return 0!=1; }'
assert 0 'int main() { return 42!=42; }'

assert 1 'int main() { return 0<1; }'
assert 0 'int main() { return 1<1; }'
assert 0 'int main() { return 2<1; }'
assert 1 'int main() { return 0<=1; }'
assert 1 'int main() { return 1<=1; }'
assert 0 'int main() { return 2<=1; }'

assert 1 'int main() { return 1>0; }'
assert 0 'int main() { return 1>1; }'
assert 0 'int main() { return 1>2; }'
assert 1 'int main() { return 1>=0; }'
assert 1 'int main() { return 1>=1; }'
assert 0 'int main() { return 1>=2; }'

assert 3 'int main() { int a; a=3; return a; }'
assert 3 'int main() { int a=3; return a; }'
assert 8 'int main() { int a=3; int z=5; return a+z; }'

assert 1 'int main() { return 1; 2; 3; }'
assert 2 'int main() { 1; return 2; 3; }'
assert 3 'int main() { 1; 2; return 3; }'

assert 3 'int main() { int a=3; return a; }'
assert 8 'int main() { int a=3; int z=5; return a+z; }'
assert 6 'int main() { int a; int b; a=b=3; return a+b; }'
assert 3 'int main() { int foo=3; return foo; }'
assert 8 'int main() { int foo123=3; int bar=5; return foo123+bar; }'

assert 3 'int main() { if (0) return 2; return 3; }'
assert 3 'int main() { if (1-1) return 2; return 3; }'
assert 2 'int main() { if (1) return 2; return 3; }'
assert 2 'int main() { if (2-1) return 2; return 3; }'

assert 55 'int main() { int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
assert 3 'int main() { for (;;) return 3; return 5; }'

assert 10 'int main() { int i=0; while(i<10) i=i+1; return i; }'

assert 3 'int main() { {1; {2;} return 3;} }'

assert 10 'int main() { int i=0; while(i<10) i=i+1; return i; }'
assert 55 'int main() { int i=0; int j=0; while(i<=10) {j=i+j; i=i+1;} return j; }'

assert 3 'int main() { int x=3; return *&x; }'
assert 3 'int main() { int x=3; int *y=&x; int **z=&y; return **z; }'
assert 5 'int main() { int x=3; int y=5; return *(&x+1); }'
#assert 3 'int main() { int x=3; int y=5; return *(&y-1); }'
#assert 5 'int main() { int x=3; int *y=&x; *y=5; return x; }'
#assert 7 'int main() { int x=3; int y=5; *(&x+1)=7; return y; }'
#assert 7 'int main() { int x=3; int y=5; *(&y-1)=7; return x; }'
#assert 2 'int main() { int x=3; return (&x+2)-&x; }'

assert 32 'int main() { return ret32(); } int ret32() { return 32; }'
assert 7 'int main() { return add2(3,4); } int add2(int x, int y) { return x+y; }'
assert 1 'int main() { return sub2(4,3); } int sub2(int x, int y) { return x-y; }'
assert 55 'int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'

assert 3 'int main() { int x[2]; int *y=&x; *y=3; return *x; }'

assert 3 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *x; }'
assert 4 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1); }'
assert 5 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2); }'

assert 0 'int main() { int x[2][3]; int *y=x; *y=0; return **x; }'
assert 1 'int main() { int x[2][3]; int *y=x; *(y+1)=1; return *(*x+1); }'
assert 2 'int main() { int x[2][3]; int *y=x; *(y+2)=2; return *(*x+2); }'
assert 3 'int main() { int x[2][3]; int *y=x; *(y+3)=3; return **(x+1); }'
assert 4 'int main() { int x[2][3]; int *y=x; *(y+4)=4; return *(*(x+1)+1); }'
assert 5 'int main() { int x[2][3]; int *y=x; *(y+5)=5; return *(*(x+1)+2); }'

assert 3 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *x; }'
assert 4 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+1); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; 2[x]=5; return *(x+2); }'

assert 0 'int main() { int x[2][3]; int *y=x; y[0]=0; return x[0][0]; }'
assert 1 'int main() { int x[2][3]; int *y=x; y[1]=1; return x[0][1]; }'
assert 2 'int main() { int x[2][3]; int *y=x; y[2]=2; return x[0][2]; }'
assert 3 'int main() { int x[2][3]; int *y=x; y[3]=3; return x[1][0]; }'
assert 4 'int main() { int x[2][3]; int *y=x; y[4]=4; return x[1][1]; }'
assert 5 'int main() { int x[2][3]; int *y=x; y[5]=5; return x[1][2]; }'

assert 4 'int main() { int x; return sizeof(x); }'
assert 4 'int main() { int x; return sizeof x; }'
assert 8 'int main() { int *x; return sizeof(x); }'
assert 16 'int main() { int x[4]; return sizeof(x); }'
assert 48 'int main() { int x[3][4]; return sizeof(x); }'
assert 16 'int main() { int x[3][4]; return sizeof(*x); }'
assert 4 'int main() { int x[3][4]; return sizeof(**x); }'
assert 5 'int main() { int x[3][4]; return sizeof(**x) + 1; }'
assert 5 'int main() { int x[3][4]; return sizeof **x + 1; }'
assert 4 'int main() { int x[3][4]; return sizeof(**x + 1); }'
assert 4 'int main() { int x=1; return sizeof(x=2); }'
assert 1 'int main() { int x=1; sizeof(x=2); return x; }'

assert 0 'int x; int main() { return x; }'
assert 3 'int x; int main() { x=3; return x; }'
assert 7 'int x; int y; int main() { int x=3; int y=4; return x+y; }'
assert 7 'int x, y; int main() { x=3; y=4; return x+y; }'
assert 0 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[0]; }'
assert 1 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[1]; }'
assert 2 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[2]; }'
assert 3 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[3]; }'

assert 4 'int x; int main() { return sizeof(x); }'
assert 16 'int x[4]; int main() { return sizeof(x); }'

assert 1 'int main() { char x=1; return x; }'
assert 1 'int main() { char x=1; char y=2; return x; }'
assert 2 'int main() { char x=1; char y=2; return y; }'

assert 1 'int main() { char x; return sizeof(x); }'
assert 10 'int main() { char x[10]; return sizeof(x); }'
assert 1 'int main() { return sub_char(7, 3, 3); } int sub_char(char a, char b, char c) { return a-b-c; }'

assert 97 'int main() { return "abc"[0]; }'
assert 98 'int main() { return "abc"[1]; }'
assert 99 'int main() { return "abc"[2]; }'
assert 0 'int main() { return "abc"[3]; }'
assert 4 'int main() { return sizeof("abc"); }'

echo OK

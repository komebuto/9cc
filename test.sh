#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    gcc -o tmp tmp.s function.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
	    echo "$input => $actual"
    else
	    echo "$input => $expected expected, but got $actual"
	    exit 1
    fi
}
assert 8 """
int main () {
    int *p;
    alloc4(&p, 1, 2, 4, 8);
    int *q;
    q = p + 3;
    return *q;
}
"""
assert 3 """
int main () {
    int x;
    int *y;
    y = &x;
    *y = 3;
    return x;
}
"""
assert 1 """
int main () {
    int x;
    x = 1;
    return x;
}
"""
assert 1 """
int main() {
    int x;
    int y;
    x = 1;
    y = &x;
    return *y;
}
"""
assert 3 """
int plus(int x,int y){
    return x+y;
}
int main() {
    int x;
    int y;
    int z;
    x = 1;
    y = 2;
    z = plus(x, y);
    return z;
}
"""
assert 0 """
int fib(int p, int pp) {
    return p + pp;
}
int main() {
    int pp;
    int p;
    int now;
    int n;
    pp = 0;
    p = 1;
    for (n = 0; n < 20; n = n+1) {
        now = fib(p, pp);
        print_int(now);
        pp = p;
        p = now;
    }
    return 0;
}
"""
assert 0 "int main(){return 0;}"
assert 1 """
int main () {
1 ==1;
}
"""
assert 0 """
int main () {
1!=1;
}
"""
assert 1 """
int main () {2>1;}
"""
assert 1 """
int main(){2>=2;}
"""
assert 0 """
int main(){2<1;}
"""
assert 0 """
int main(){2<=1;}
"""
assert 5  """
int main(){
    int a1;
    int b_;
    a1 = -12 + (2 - 2 * 3) / 2 + 17;
    b_ = + 2; 
    return a1 + b_;
}
"""
assert 1 """
int main(){
    int return1;
    return1 = 1;
    return return1;
}
"""
assert 1 """
int main(){
    int a;
    a = 1;
    if (a == 0) 
        return 2;
    else 
        return 1;
}
"""
assert 2 """
int main(){
    int a;
    a = 1;
    if (a == 1) 
        return 2;
    else 
        return 1;
}
"""
assert 10 """
int main(){
    int n;
    int a;
    n = 0;
    for (a=0; a<10; a=a+1)
        n = n + 1;
    return n;
}
"""
assert 0 """
int main(){
    int n;
    int a;
    n = 0;
    for (a=0; a<0; a=a+1)
        n = n + 1;
    return n;
}
"""
assert 1 """
int main(){
    int n;
    n = 1;
    while(0)
        return n+1;
    return n;
}
"""
assert 2 """
int main(){
    int n;
    n = 1;
    while(1)
        return n+1;
    return n;
}
"""
assert 5 """
int main(){
    int n;
    n = 1;
    while (n <= 10) {
        if (n == 5) {
            return n;
        }
        n = n+1;
    }
    return n;
}
"""
assert 10 """
int main(){
    int n;
    n = 1;
    while (n < 10) {
        if (n == 20) {
            return n;
        }
        n = n+1;
    }
    return n;
}
"""
assert 1 """
int main(){
    int foo;
    foo = 1;
    foo();
    return foo;
}
"""
assert 1 """
int main(){
    int foo;
    foo = 0;
    foo = foo();
    return foo;
}
"""
assert 1 "int main () {return 1;}"
assert 44 """
int main(){
    int a;int b;int c;int d;int e;int f;int g;
    a=1;b=2;c=3;d=4;e=5;f=6;
    g=mpmpm(a,b,c,d,e,f);
    return g;
}
"""
assert 11 """
int main(){
int a;int b;int c;int d;int e;int f;
a=1; b=2; c=3; d=2; e=3;
f = mpmp(a,b,c,d,e);
return f;
}
"""
assert 14 """
int main(){
    int x;int y;int z;int u;int w;
x = 1;
y = 2;
z = 3;
u = 4;
w = mpm(x, y, z, u);
return w;
}
"""
assert 14 """
int main(){
    int x;int y;int z;
x = 1;
y = 2;
z = 3;
return mpm(x, y, z, 4);
}
"""
assert 5 """
int main(){
    int a; int b; int c; int d;
a = 1; b=2; c=3;
d = mp(a, b, c);
return d;
}
"""
assert 5 """
int main(){int u;return u = mp(1, 2, 3);}
"""
assert 3 """
int main(){
    int x; int y; int z;
x = 1;
y = 2;
z = p(x, y);
return z;
}
"""
assert 2 """
int main(){
    int x;
x = 1;
return twice(x);
}
"""

echo OK

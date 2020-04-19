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
assert 44 """
a=1;
b=2;
c=3;
d=4;
e=5;
f=6;
g=mpmpm(a,b,c,d,e,f);
return g;
"""
assert 11 """
a=1; b=2; c=3; d=2; e=3;
f = mpmp(a,b,c,d,e);
return f;
"""
assert 14 """
x = 1;
y = 2;
z = 3;
u = 4;
w = mpm(x, y, z, u);
return w;
"""
assert 14 """
x = 1;
y = 2;
z = 3;
return mpm(x, y, z, 4);
"""
assert 5 """
a = 1; b=2; c=3;
d = mp(a, b, c);
return d;
"""
assert 5 """
return u = mp(1, 2, 3);
"""
assert 3 """
x = 1;
y = 2;
z = p(x, y);
return z;
"""
assert 2 """
x = 1;
return twice(x);
"""
assert 0 "0;"
assert 1 """
1 ==1;
"""
assert 0 """
1!=1;
"""
assert 1 """
2>1;
"""
assert 1 """
2>=2;
"""
assert 0 """
2<1;
"""
assert 0 """
2<=1;
"""
assert 5  """
a1 = -12 + (2 - 2 * 3) / 2 + 17;
b_ = + 2; 
return a1 + b_;
"""
assert 1 """
return1 = 1;
return return1;
"""
assert 1 """
a = 1;
if (a == 0) 
    return 2;
else 
    return 1;
"""
assert 2 """
a = 1;
if (a == 1) 
    return 2;
else 
    return 1;
"""
assert 10 """
n = 0;
for (a=0; a<10; a=a+1)
    n = n + 1;
return n;
"""
assert 0 """
n = 0;
for (a=0; a<0; a=a+1)
    n = n + 1;
return n;
"""
assert 1 """
n = 1;
while(0)
    return n+1;
return n;
"""
assert 2 """
n = 1;
while(1)
    return n+1;
return n;
"""
assert 5 """
n = 1;
while (n <= 10) {
    if (n == 5) {
        return n;
    }
    n = n+1;
}
return n;
"""
assert 10 """
n = 1;
while (n < 10) {
    if (n == 20) {
        return n;
    }
    n = n+1;
}
return n;
"""
assert 1 """
foo = 1;
foo();
return foo;
"""
assert 1 """
foo = 0;
foo = foo();
return foo;
"""

echo OK

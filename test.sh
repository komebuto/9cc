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
assert 0 """
foo();
"""

echo OK

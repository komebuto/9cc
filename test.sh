#!/bin/bash
assert(){
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    gcc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
	echo "$input => $actual"
    else
	echo "$input => $expected expected, but got $actual"
	exit 1
    fi
}

assert 0 0
assert 42 42
assert 21 "5+20-4"
assert 34 " 12 + 32 - 10 "
assert 25 " 12 + (2 - 2 * 3) + 17"
assert 4 "(3+5)/2"
assert 10 "-10+20"
assert 20 "+10+10"

echo OK

#!/bin/bash

assert() {
	input=$1
	expected=$2

	./ppcc "$input" > tmp.s
	if [ "$?" == "1" ]; then
		echo "ppcc failed: "
		cat tmp.s
		exit 1
	fi

	clang --target=powerpc-unknown-gnu -fuse-ld=lld -static tmp.s -o tmp
	qemu-ppc ./tmp
	actual="$?"

	if [ "$expected" = "$actual" ]; then
		echo "$input => $actual, pass"
	else
		echo "$input => $actual, $expected expected got $actual, failed"
		exit 1
	fi

	rm -f ./tmp
}

assert '{ return 0; }' 0
assert '{ 99; }' 99
assert '{ 9+12-8; }' 13
assert '{  5  + 99-      8   ; }' 96
assert '{ (3+5)-2; }' 6
assert '{ 4+2*3; }' 10
assert '{ (3+5)/2; }' 4
assert '{ 5*(9-6); }' 15
assert '{ - - + 5; }' 5
assert '{ -5 * -5; }' 25
assert '{ 5 != 5; }' 0
assert '{ 4 != 42; }' 1
assert '{ 5 == 5; }' 1
assert '{ 4 == 5; }' 0
assert '{ 99 > 4; }' 1
assert '{ 4 > 1000; }' 0
assert '{ 4 < 23;}' 1
assert '{ 99 < 28;}' 0
assert '{ 3 >= 3;}' 1
assert '{ 99 >= 98;}' 1
assert '{ 3 >= 23;}' 0

assert '{ 0<1; }' 1
assert '{ 1<1; }' 0
assert '{ 2<1; }' 0
assert '{ 0<=1; }' 1

assert '{ 1<=1; }' 1
assert '{ 2<=1; }' 0

assert '{1>0; }' 1
assert '{1>1; }' 0
assert '{1>2; }' 0
assert '{1>=0; }' 1
assert '{1>=1; }' 1
assert '{1>=2; }' 0

assert '{1; 2; 3; 4; }' 4

assert '{ a=3; return a; }' 3
assert '{ a=3; z=5; return a+z; }' 8
assert '{ a=b=3; return a+b; }' 6

assert '{ foo= 3; foo; }' 3
assert '{ foo123=3; bar=5; foo123+bar; }' 8

assert '{ return 1; 2; 3; }' 1
assert '{ 1; return 2; 3; }' 2
assert '{ 1; 2; return 3; }' 3

assert '{ {1; {2;} return 3; } }' 3
assert '{ ;;; return 3; }' 3

assert '{ if (0) return 2; return 3; }' 3
assert '{ if (1-1) return 2; return 3; }' 3
assert '{ if (1) return 2; return 3; }' 2
assert '{ if (2-1) return 2; return 3; }' 2
assert '{ if (0) { 1; 2; return 3; } else { return 4; } }' 4
assert '{ if (1) { 1; 2; return 3; } else { return 4; } }' 3

assert '{ i=0; j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }' 55
assert '{ for (;;) {return 3;} return 5; }' 3

echo OK

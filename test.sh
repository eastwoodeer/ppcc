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

assert 0 0
assert 99 99
assert '9+12-8' 13
assert ' 5  + 99-      8   ' 96
assert '(3+5)-2' 6
assert '4+2*3' 10
assert '(3+5)/2' 4
assert '5*(9-6)' 15
assert '- - + 5' 5
assert '-5 * -5' 25


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

	powerpc-linux-gnu-gcc -static -o tmp tmp.s
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

#!/bin/bash

assert() {
	expected=$2
	input=$1

	./ppcc "$input" > tmp.s
	powerpc-linux-gnu-gcc -static -o tmp tmp.s
	qemu-ppc ./tmp
	actual="$?"

	if [ "$input" = "$actual" ]; then
		echo "$input => $actual, pass"
	else
		echo "$input => $actual, $expected expected got $actual, failed"
		exit 1
	fi
}

assert 0 0
assert 99 99

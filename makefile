build:
	gcc f16.c test.c -o test

test: build
	./test
CC = g++
LEX = flex
CFLAGS = -Wall

all: output

output: lex.yy.c
	$(CC) lex.yy.c -o *.cpp

lex.yy.c: rules.l
	$(LEX) rules.l

run: output
	./output < test.s

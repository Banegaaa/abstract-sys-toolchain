CC = g++
LEX = flex
CFLAGS = -Wall
# I need a list of all .cpp files that make asembler, those are: mainASM.cpp, firstPass.cpp, literal.cpp, secondPass.cpp, symbol.cpp
ASM_FILES = ./src/mainASM.cpp ./src/firstPass.cpp ./src/literal.cpp ./src/secondPass.cpp ./src/symbol.cpp ./src/relocSymbol.cpp
LINKER_FILES = ./src/mainLD.cpp ./src/linker.cpp ./src/parserLD.cpp

all: output

asembler: lex.yy.c
	$(CC) lex.yy.c -o asembler $(ASM_FILES)

linker: lex.yy.c
	$(CC) lex.yy.c -o linker $(LINKER_FILES)
lex.yy.c: rules.l
	$(LEX) rules.l

run: output
	./output < test.s

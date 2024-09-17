
src/lexer.cpp: misc/lexer.l #ADD incl path
	flex $< \
	&& mv lexer.cpp src/lexer.cpp \
	&& mv lexer.hpp inc/lexer.hpp

src/parser.cpp: misc/parser.y misc/lexer.l | build #ADD incl path
	bison -W -v $< \
	&& mv parser.cpp src/parser.cpp \
	&& mv parser.hpp inc/parser.hpp \
	&& mv parser.output build/parser.output

build:
	mkdir -p build

assemble: src/lexer.cpp src/parser.cpp
	g++ src/lexer.cpp src/parser.cpp src/Assembler.cpp  $(CFLAGS) -o build/assemble

CFLAGS= -std=c++11 -Wall -Wextra -o3

lexAndParse:  src/parser.cpp src/lexer.cpp
	g++ src/parser.cpp src/lexer.cpp $(CFLAGS) -o build/lexAndParse


clean:
	rm -rf *.o src/lexer.cpp inc/lexer.hpp src/parser.cpp inc/parser.hpp build/parser.output build

# snazzle.tab.c snazzle.tab.h: bison.y
#   bison -d bison.y

# lex.yy.c: snazzle.l snazzle.tab.h
#   flex snazzle.l

# snazzle: lex.yy.c snazzle.tab.c snazzle.tab.h
#   g++ snazzle.tab.c lex.yy.c -lfl -o snazzle
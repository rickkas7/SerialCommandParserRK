

all : ParserTest
	./ParserTest -ip

ParserTest : ParserTest.cpp ../src/SerialCommandParserRK.cpp ../src/SerialCommandParserRK.h libwiringgcc
	gcc ParserTest.cpp ../src/SerialCommandParserRK.cpp gcclib/libwiringgcc.a -std=c++11 -lc++ -Igcclib -I../src -DUNITTEST -o ParserTest

check : ParserTest.cpp ../src/SerialCommandParserRK.cpp ../src/SerialCommandParserRK.h libwiringgcc
	gcc ParserTest.cpp ../src/SerialCommandParserRK.cpp gcclib/libwiringgcc.a -g -O0 -std=c++11 -lc++ -Igcclib -I ../src -o ParserTest && valgrind --leak-check=yes ./ParserTest 

libwiringgcc :
	cd gcclib && make libwiringgcc.a 	
	
.PHONY: libwiringgcc
#include <unistd.h>

#include "Particle.h"
#include "SerialCommandParserRK.h"

char *readTestData(const char *filename) {
	char *data;

	FILE *fd = fopen(filename, "r");
	if (!fd) {
		printf("failed to open %s", filename);
		return 0;
	}

	fseek(fd, 0, SEEK_END);
	size_t size = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	data = (char *) malloc(size + 1);
	fread(data, 1, size, fd);
	data[size] = 0;

	fclose(fd);

	return data;
}

void _assertString(const char *expected, const char *got, int line) {
	if (!expected) {
		fprintf(stderr, "null expected pointer at line %d\n", line);
		return;
	}
	if (!got) {
		fprintf(stderr, "null got pointer at line %d\n", line);
		return;
	}
	if (strcmp(got, expected)) {
		fprintf(stderr, "non matching string got '%s' expected '%s' at line %d\n", got, expected, line);
	}
}
#define assertString(e, g) _assertString(e, g, __LINE__)

void _assertInt(int expected, int got, int line) {
	if (expected != got) {
		fprintf(stderr, "non matching value got '%d' expected '%d' at line %d\n", got, expected, line);
	}
}
#define assertInt(e, g) _assertInt((int)e, (int)g, __LINE__)

void _assertFloat(float expected, float got, float margin, int line) {
	if ((got < (expected - margin)) || (got > (expected + margin))) {
		fprintf(stderr, "non matching value got '%f' expected '%f' at line %d\n", got, expected, line);
	}
}
#define assertFloat(e, g, m) _assertFloat(e, g, m, __LINE__)

void parserUnitTest();
void interactiveTest();

int main(int argc, char *argv[]) {
	int c;
	bool interactiveMode = false;

	while ((c = getopt (argc, argv, "ip")) != -1) {
		switch(c) {
		case 'i':
			interactiveMode = true;
			break;
		case 'p':
			parserUnitTest();
			break;
		}
	}

	if (interactiveMode) {
		interactiveTest();
	}

	return 0;
}

void parserUnitTest() {
	{
		SerialCommandParser<100, 10> parser;

		parser.processString("test");
		assertString("test", parser.getBuffer());

		parser.deleteCharacterAt(0);
		assertString("est", parser.getBuffer());

		parser.deleteCharacterAt(1);
		assertString("et", parser.getBuffer());

		parser.deleteCharacterAt(2);
		assertString("et", parser.getBuffer());

		parser.deleteCharacterAt(1);
		assertString("e", parser.getBuffer());

		parser.deleteCharacterAt(0);
		assertString("", parser.getBuffer());

		parser.processString("test");
		assertString("test", parser.getBuffer());

		parser.deleteToEnd(10);
		assertString("test", parser.getBuffer());

		parser.deleteToEnd(2);
		assertString("te", parser.getBuffer());

		parser.clear();

		parser.processString("test");
		assertString("test", parser.getBuffer());

		parser.deleteCharacterLeft(0);
		assertString("test", parser.getBuffer());

		parser.deleteCharacterLeft(1);
		assertString("est", parser.getBuffer());

		parser.deleteCharacterLeft(3);
		assertString("es", parser.getBuffer());

		parser.deleteCharacterLeft(1);
		assertString("s", parser.getBuffer());

		parser.deleteCharacterLeft(1);
		assertString("", parser.getBuffer());

		parser.processString("test");
		assertString("test", parser.getBuffer());

		parser.insertCharacterAt(0, 'a');
		assertString("atest", parser.getBuffer());

		parser.insertCharacterAt(5, 'z');
		assertString("atestz", parser.getBuffer());

		parser.insertCharacterAt(1, 'b');
		assertString("abtestz", parser.getBuffer());

		parser.insertCharacterAt(6, 'y');
		assertString("abtestyz", parser.getBuffer());

	}

	{
		SerialCommandParser<100, 10> parser;

		parser.processString("test");
		parser.processLine();

		assertInt(1, parser.getArgsCount());
		assertString("test", parser.getArgsBuffer()[0]);
	}

	{
		SerialCommandParser<100, 10> parser;

		parser.processString("a b");
		parser.processLine();

		assertInt(2, parser.getArgsCount());
		assertString("a", parser.getArgsBuffer()[0]);
		assertString("b", parser.getArgsBuffer()[1]);
	}

	{
		SerialCommandParser<100, 10> parser;

		parser.processString("  a  b   ");
		parser.processLine();

		assertInt(2, parser.getArgsCount());
		assertString("a", parser.getArgsBuffer()[0]);
		assertString("b", parser.getArgsBuffer()[1]);
	}

	{
			SerialCommandParser<100, 10> parser;

			parser.processString("\t a\tb   \t");
			parser.processLine();

			assertInt(2, parser.getArgsCount());
			assertString("a", parser.getArgsBuffer()[0]);
			assertString("b", parser.getArgsBuffer()[1]);
		}

	{
		SerialCommandParser<100, 10> parser;

		parser.processString("test \"aa bb\"");
		parser.processLine();

		assertInt(2, parser.getArgsCount());
		assertString("test", parser.getArgsBuffer()[0]);
		assertString("aa bb", parser.getArgsBuffer()[1]);
	}

	{
		SerialCommandParser<100, 10> parser;

		parser.processString("test 'aa bb'");
		parser.processLine();

		assertInt(2, parser.getArgsCount());
		assertString("test", parser.getArgsBuffer()[0]);
		assertString("aa bb", parser.getArgsBuffer()[1]);
	}

	{
		SerialCommandParser<100, 10> parser;

		parser.processString("test aa\\ bb");
		parser.processLine();

		assertInt(2, parser.getArgsCount());
		assertString("test", parser.getArgsBuffer()[0]);
		assertString("aa bb", parser.getArgsBuffer()[1]);
	}

	{
		SerialCommandParser<100, 10> parser;

		parser.processString("test aa\"bb cc\"");
		parser.processLine();

		assertInt(2, parser.getArgsCount());
		assertString("test", parser.getArgsBuffer()[0]);
		assertString("aabb cc", parser.getArgsBuffer()[1]);
	}

	{
		SerialCommandParser<100, 10> parser;

		parser.processString("test aa'bb cc'dd");
		parser.processLine();

		assertInt(2, parser.getArgsCount());
		assertString("test", parser.getArgsBuffer()[0]);
		assertString("aabb ccdd", parser.getArgsBuffer()[1]);
	}

	{
		SerialCommandParser<100, 10> parser;

		parser.processString("test aa\\'bb cc");
		parser.processLine();

		assertInt(3, parser.getArgsCount());
		assertString("test", parser.getArgsBuffer()[0]);
		assertString("aa'bb", parser.getArgsBuffer()[1]);
		assertString("cc", parser.getArgsBuffer()[2]);
	}
	{
		SerialCommandParser<100, 4> parser;

		parser.processString("a b c d e");
		parser.processLine();

		assertInt(4, parser.getArgsCount());
		assertString("a", parser.getArgsBuffer()[0]);
		assertString("b", parser.getArgsBuffer()[1]);
		assertString("c", parser.getArgsBuffer()[2]);
		assertString("d", parser.getArgsBuffer()[3]);
	}
	{
		SerialCommandParser<10, 4> parser;

		parser.processString("0123456789abc");
		parser.processLine();

		assertInt(1, parser.getArgsCount());
		assertString("012345678", parser.getArgsBuffer()[0]);
	}

	{
		SerialCommandParser<100, 10> parser;

		parser.processString("a b");
		parser.processLine();

		assertInt(2, parser.getArgsCount());
		assertString("a", parser.getArgsBuffer()[0]);
		assertString("b", parser.getArgsBuffer()[1]);

		parser.clear();
		parser.processString("c d e");
		parser.processLine();

		assertInt(3, parser.getArgsCount());
		assertString("c", parser.getArgsBuffer()[0]);
		assertString("d", parser.getArgsBuffer()[1]);
		assertString("e", parser.getArgsBuffer()[2]);
	}

	{
		SerialCommandParser<100, 10> parser;

		parser.processString("aaa 123 -345 10.5 y 0");
		parser.processLine();

		assertInt(6, parser.getArgsCount());
		assertString("aaa", parser.getArgString(0));
		assertString("123", parser.getArgString(1));
		assertInt(123, parser.getArgInt(1));
		assertInt(-345, parser.getArgInt(2));

		assertFloat(10.5, parser.getArgFloat(3), 0.001);

		assertInt(true, parser.getArgBool(4));
		assertInt(false, parser.getArgBool(5));

		assertInt(0, parser.getArgBool(6));
	}

	{
		SerialCommandEditor<50, 50, 10> parser;

		assertInt(0, parser.historySize());

		parser.historyAdd("012345678");
		assertInt(1, parser.historySize());
		assertString("012345678", parser.historyGet(0));

		parser.historyAdd("a12345678");
		assertInt(2, parser.historySize());
		assertString("a12345678", parser.historyGet(0));
		assertString("012345678", parser.historyGet(1));

		parser.historyAdd("b12345678");
		assertInt(3, parser.historySize());
		assertString("b12345678", parser.historyGet(0));
		assertString("a12345678", parser.historyGet(1));
		assertString("012345678", parser.historyGet(2));

		parser.historyAdd("c12345678");
		assertInt(4, parser.historySize());
		assertString("c12345678", parser.historyGet(0));
		assertString("b12345678", parser.historyGet(1));
		assertString("a12345678", parser.historyGet(2));
		assertString("012345678", parser.historyGet(3));

		parser.historyAdd("d1234567"); // <- one less to make sure it exactly fills the buffer with trailing null
		assertInt(5, parser.historySize());
		assertString("d1234567", parser.historyGet(0));
		assertString("c12345678", parser.historyGet(1));
		assertString("b12345678", parser.historyGet(2));
		assertString("a12345678", parser.historyGet(3));
		assertString("012345678", parser.historyGet(4));

		parser.historyAdd("e12345678");
		assertInt(5, parser.historySize());
		assertString("e12345678", parser.historyGet(0));
		assertString("d1234567", parser.historyGet(1));
		assertString("c12345678", parser.historyGet(2));
		assertString("b12345678", parser.historyGet(3));
		assertString("a12345678", parser.historyGet(4));

		parser.historyRemoveFirst();
		assertInt(4, parser.historySize());
		assertString("d1234567", parser.historyGet(0));
		assertString("c12345678", parser.historyGet(1));
		assertString("b12345678", parser.historyGet(2));
		assertString("a12345678", parser.historyGet(3));

		parser.historyAdd("f12345678012345678012345678");
		assertInt(3, parser.historySize());
		assertString("f12345678012345678012345678", parser.historyGet(0));
		assertString("d1234567", parser.historyGet(1));
		assertString("c12345678", parser.historyGet(2));

	}

	printf("paserUnitTest complete!\n");

}



void interactiveTest() {
	printf("running interactive test\n");

	SerialCommandParser<256, 16> parser;
	char lineBuf[256];
	bool done = false;

	parser.addCommandHandler("test", "test command", [&parser](SerialCommandParserBase *) {
		printf("got test command!\n");
		for(size_t ii = 0; ii < parser.getArgsCount(); ii++) {
			printf("arg %lu: %s\n", ii, parser.getArgString(ii));
		}
	});
	parser.addCommandHandler("quit|exit", "exit interactive test", [&done](SerialCommandParserBase *) {
		done = true;
	});
	parser.addHelpCommand();

	while(!done) {
		char *s = fgets(lineBuf, sizeof(lineBuf), stdin);
		if (!s) {
			break;
		}
		parser.processString(lineBuf);
	}
}

void printIndent(size_t indent) {
	for(size_t ii = 0; ii < 2 * indent; ii++) {
		printf(" ");
	}
}

void printString(const char *str) {
	printf("\"");

	for(size_t ii = 0; str[ii]; ii++) {
		if (str[ii] == '"') {
			printf("\\\"");
		}
		else
		if (str[ii] == '\\') {
			printf("\\\\");
		}
		else
		if (str[ii] >= 32 && str[ii] < 127) {
			printf("%c", str[ii]);
		}
		else {
			printf("\\x%02x", str[ii]);
		}
	}
	printf("\"");
}




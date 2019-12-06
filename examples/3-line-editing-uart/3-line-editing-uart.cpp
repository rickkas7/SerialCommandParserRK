#include "SerialCommandParserRK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

SerialCommandEditor<1000, 256, 16> commandParser;

void setup() {
	Serial1.begin(9600);

	commandParser
		.withSerial(&Serial1)
		.withPrompt("> ")
		.setup();

	commandParser.addCommandHandler("test", "test command", [](SerialCommandParserBase *) {
		Serial.println("got test command!");
		for(size_t ii = 0; ii < commandParser.getArgsCount(); ii++) {
			Serial.printlnf("arg %u: '%s'", ii, commandParser.getArgString(ii));
		}
	});
	commandParser.addHelpCommand();
}

void loop() {
	commandParser.loop();
}

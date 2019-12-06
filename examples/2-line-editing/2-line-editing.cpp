#include "SerialCommandParserRK.h"

SYSTEM_THREAD(ENABLED);

// Note: Logs to Serial1 (hardware serial) to avoid mixing the output into USB serial that we're
// using for editing mode.
Serial1LogHandler logHandler(LOG_LEVEL_INFO);

SerialCommandEditor<1000, 256, 16> commandParser;


void setup() {
	Serial.begin();

	commandParser
		.withSerial(&Serial)
		.withPrompt("> ")
		.withWelcome("Serial Command Parser Test!")
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

#include "SerialCommandParserRK.h"

SYSTEM_THREAD(ENABLED);

// Note: Logs to Serial1 (hardware serial) to avoid mixing the output into USB serial that we're
// using for editing mode.
Serial1LogHandler logHandler(LOG_LEVEL_INFO);

SerialCommandEditor<1000, 256, 16> commandParser;


void setup() {
	Serial.begin();

	// Configuration of prompt and welcome message
	commandParser
		.withPrompt("> ")
		.withWelcome("Serial Command Parser Test!");

	// Command configuration
	commandParser.addCommandHandler("test", "test command", [](SerialCommandParserBase *) {
		commandParser.printMessageNoPrompt("got test command!");
		for(size_t ii = 0; ii < commandParser.getArgsCount(); ii++) {
			commandParser.printMessageNoPrompt("  arg %u: '%s'", ii, commandParser.getArgString(ii));
		}
		commandParser.printMessagePrompt();
	});

	commandParser.addCommandHandler("foo", "foo command", [](SerialCommandParserBase *) {
		commandParser.printMessage("got foo command!");
	});

	commandParser.addCommandHandler("aaaa", "aaaa command", [](SerialCommandParserBase *) {
		commandParser.printMessage("got aaaa command!");
	});
	commandParser.addCommandHandler("aaabbbb", "aaabbbb command", [](SerialCommandParserBase *) {
		commandParser.printMessage("got aaabbbb command!");
	});

	commandParser.addHelpCommand();

	// Connect to Serial and start running
	commandParser
		.withSerial(&Serial)
		.setup();

}

void loop() {
	commandParser.loop();
}

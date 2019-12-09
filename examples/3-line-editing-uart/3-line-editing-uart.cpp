#include "SerialCommandParserRK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

SerialCommandEditor<1000, 256, 16> commandParser;

void setup() {
	Serial1.begin(9600);

	// Set prompt. Welcome message is not supported on UART serial
	commandParser
		.withPrompt("> ");

	// Add command handlers
	commandParser.addCommandHandler("test", "test command", [](SerialCommandParserBase *) {
		Serial.println("got test command!");
		for(size_t ii = 0; ii < commandParser.getArgsCount(); ii++) {
			Serial.printlnf("arg %u: '%s'", ii, commandParser.getArgString(ii));
		}
	});
	commandParser.addHelpCommand();

	// Connect to Serial1 and start running
	commandParser
		.withSerial(&Serial1)
		.setup();

}

void loop() {
	commandParser.loop();
}

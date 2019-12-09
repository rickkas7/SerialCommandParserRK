#include "SerialCommandParserRK.h"

// Basically the 2-line-editing example, but also interleaves the system log messages

SYSTEM_THREAD(ENABLED);

SerialCommandEditor<1000, 256, 16> commandParser;

// This sets up a log handler with a 1024 byte buffer. This buffer must be big enough to hold
// the maximum amount of data between calls to logHandler.loop().
// Be sure to call logHandler.setup() and logHandler.loop() from setup() and loop()!
SerialCommandEditorLogHandler<1024> logHandler(&commandParser, LOG_LEVEL_INFO);

const unsigned long LOG_PERIOD_MS = 4000;
unsigned long lastLog = 0;
int counter = 0;


void setup() {
	Serial.begin();

	logHandler.setup();

	// Common configuration settings
	commandParser
		.withPrompt("test> ")
		.withWelcome("Serial Command Parser Test!");

	// Command handlers
	commandParser.addCommandHandler("test", "test command", [](SerialCommandParserBase *) {
		Serial.println("got test command!");
		for(size_t ii = 0; ii < commandParser.getArgsCount(); ii++) {
			Serial.printlnf("arg %u: '%s'", ii, commandParser.getArgString(ii));
		}
	});

	// Help
	commandParser.addHelpCommand();

	// Final setup
	commandParser
		.withSerial(&Serial)
		.setup();

}

void loop() {
	logHandler.loop();
	commandParser.loop();

	if (millis() - lastLog >= LOG_PERIOD_MS) {
		lastLog = millis();
		int tries = rand() % 5;
		for(int ii = 0; ii < tries; ii++) {
			Log.info("log counter=%d ii=%d", ++counter, ii);
		}
	}
}


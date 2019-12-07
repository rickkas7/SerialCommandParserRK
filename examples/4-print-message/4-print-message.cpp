#include "SerialCommandParserRK.h"

// Basically the 2-line-editing example, but also periodically prints messages to test the
// correct interleaving of editing and display

SYSTEM_THREAD(ENABLED);

// Note: Logs to Serial1 (hardware serial) to avoid mixing the output into USB serial that we're
// using for editing mode.
Serial1LogHandler logHandler(LOG_LEVEL_INFO);

SerialCommandEditor<1000, 256, 16> commandParser;

const unsigned long PRINT_PERIOD_MS = 4000;
unsigned long lastPrint = 0;
int counter = 0;

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

	if (millis() - lastPrint >= PRINT_PERIOD_MS) {
		lastPrint = millis();
		counter++;

		switch(counter % 10) {
		case 8:
			commandParser.printMessage("multi-line message\nmessage counter=%d\n", counter);
			break;
		case 9:
			commandParser.printMessage("start012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789end", counter);
			break;

		default:
			commandParser.printMessage("message counter=%d", counter);
		}
	}
}


#include "SerialCommandParserRK.h"

SerialLogHandler logHandler;

SerialCommandParser<128, 16> commandParser;


void setup() {
	commandParser.withSerial(&Serial).setup();

	commandParser.addCommandHandler("test", "test command", [](SerialCommandParserBase *) {
		Log.info("got test command!");
		for(size_t ii = 0; ii < commandParser.getArgsCount(); ii++) {
			Log.info("arg %u: '%s'", ii, commandParser.getArgString(ii));
		}
	});
	commandParser.addHelpCommand();

}

void loop() {
	commandParser.loop();
}

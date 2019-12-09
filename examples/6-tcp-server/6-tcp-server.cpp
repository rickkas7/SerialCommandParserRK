#include "SerialCommandParserRK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_INFO);

SerialCommandTCPServer server(1024, 256, 16, 2, true, 5123);

void setup() {
	Serial.begin();

	// Common configuration settings
	server
		.withPrompt("test> ")
		.withWelcome("Serial Command Parser Test!");

	// Command handlers
	server.addCommandHandler("test", "test command", [](SerialCommandParserBase *parser) {
		SerialCommandEditorBase *editor = (SerialCommandEditorBase *)parser;

		editor->printMessageNoPrompt("got test command!");
		for(size_t ii = 0; ii < editor->getArgsCount(); ii++) {
			editor->printMessageNoPrompt("arg %u: '%s'", ii, editor->getArgString(ii));
		}
		editor->printMessagePrompt();
	});

	server.addCommandHandler("quit|exit", "quit session", [](SerialCommandParserBase *parser) {
		server.stop(parser);
	});

	// Help
	server.addHelpCommand();

	server.setup();

}

void loop() {
	server.loop();
}

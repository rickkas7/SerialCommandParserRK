#include "SerialCommandParserRK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_TRACE);

SerialCommandParser<1000, 16> commandParser;

void runUnitTest();

void setup() {
	Serial.begin();

	// Configuration of prompt and welcome message
	commandParser
		.withPrompt("> ")
		.withWelcome("Serial Command Parser Test!");

	// Command configuration
	commandParser.addCommandHandler("ls", "list directory", [](SerialCommandParserBase *) {
	})
    .addCommandOption('l', "long", "long format listing")
    .addCommandOption('R', "recursive", "recursive listing");

	commandParser.addCommandHandler("test1", "test1 command", [](SerialCommandParserBase *) {
	})
    .addCommandOption('v', "verbose", "increase verbosity");

	commandParser.addCommandHandler("test2", "test2 command", [](SerialCommandParserBase *) {
	})
    .addCommandOption('c', "coord", "x and y coordinates", true, 2);

	commandParser.addCommandHandler("test3", "test3 command", [](SerialCommandParserBase *) {
	})
    .addCommandOption('l', "long", "long format listing")
    .addCommandOption('R', "recursive", "recursive listing")
    .addCommandOption('v', "verbose", "increase verbosity")
    .addCommandOption('x', "x-value", "x value", false, 1);

	commandParser.addHelpCommand();

	// Connect to Serial and start running
	commandParser
		.withSerial(&Serial)
		.setup();

}

void loop() {
	commandParser.loop();

    static unsigned long lastTest = 0;
    if (millis() - lastTest >= 10000) {
        lastTest = millis();
        
        runUnitTest();
    }
}

#define assertNotNull(val) if (!(val)) { Log.error("FAILURE was null line %u", __LINE__); return; }
#define assertEqualInt(val, exp) if (val != exp) { Log.error("FAILURE %d != %d line %u", (int)(val), (int)(exp), __LINE__); return; }
#define assertEqualString(val, exp) if (strcmp(val, exp) != 0) { Log.error("FAILURE %s != %s line %u", val, exp, __LINE__); return; }

void runUnitTest() {
	Log.info("runUnitTest starting");

	{
		commandParser.clear();


		commandParser.processString("ls");
		commandParser.processLine();

		CommandParsingState *cps = commandParser.getParsingState();
		assertNotNull(cps);
		assertEqualInt(cps->getParseSuccess(), true);
		assertEqualInt(cps->getNumExtraArgs(), 0);

		assertEqualInt(cps->getByShortOpt('l'), 0);	
		assertEqualInt(cps->getByShortOpt('R'), 0);	
	}

	{
		commandParser.clear();
		commandParser.processString("ls -l");
		commandParser.processLine();

		CommandParsingState *cps = commandParser.getParsingState();
		assertNotNull(cps);
		assertEqualInt(cps->getParseSuccess(), true);
		assertEqualInt(cps->getNumExtraArgs(), 0);

		assertNotNull(cps->getByShortOpt('l'));	
		assertEqualInt(cps->getByShortOpt('l')->count, 1);
		assertEqualInt(cps->getByShortOpt('R'), 0);	
	}
	{
		commandParser.clear();
		commandParser.processString("ls --long");
		commandParser.processLine();

		CommandParsingState *cps = commandParser.getParsingState();
		assertNotNull(cps);
		assertEqualInt(cps->getParseSuccess(), true);
		assertEqualInt(cps->getNumExtraArgs(), 0);

		assertNotNull(cps->getByShortOpt('l'));	
		assertEqualInt(cps->getByShortOpt('l')->count, 1);
		assertEqualInt(cps->getByShortOpt('R'), 0);	
	}

	{
		commandParser.clear();
		commandParser.processString("ls -lR");
		commandParser.processLine();

		CommandParsingState *cps = commandParser.getParsingState();
		assertNotNull(cps);
		assertEqualInt(cps->getParseSuccess(), true);
		assertEqualInt(cps->getNumExtraArgs(), 0);

		assertNotNull(cps->getByShortOpt('l'));	
		assertEqualInt(cps->getByShortOpt('l')->count, 1);
		assertNotNull(cps->getByShortOpt('R'));	
		assertEqualInt(cps->getByShortOpt('R')->count, 1);
	}

	{
		commandParser.clear();
		commandParser.processString("ls -l -R");
		commandParser.processLine();

		CommandParsingState *cps = commandParser.getParsingState();
		assertNotNull(cps);
		assertEqualInt(cps->getParseSuccess(), true);
		assertEqualInt(cps->getNumExtraArgs(), 0);

		assertNotNull(cps->getByShortOpt('l'));	
		assertEqualInt(cps->getByShortOpt('l')->count, 1);
		assertNotNull(cps->getByShortOpt('R'));	
		assertEqualInt(cps->getByShortOpt('R')->count, 1);
	}

	{
		commandParser.clear();
		commandParser.processString("test1 -v");
		commandParser.processLine();

		CommandParsingState *cps = commandParser.getParsingState();
		assertNotNull(cps);
		assertEqualInt(cps->getParseSuccess(), true);
		assertEqualInt(cps->getNumExtraArgs(), 0);

		assertNotNull(cps->getByShortOpt('v'));	
		assertEqualInt(cps->getByShortOpt('v')->count, 1);
	}
	{
		commandParser.clear();
		commandParser.processString("test1 -vvv");
		commandParser.processLine();

		CommandParsingState *cps = commandParser.getParsingState();
		assertNotNull(cps);
		assertEqualInt(cps->getParseSuccess(), true);
		assertEqualInt(cps->getNumExtraArgs(), 0);

		assertNotNull(cps->getByShortOpt('v'));	
		assertEqualInt(cps->getByShortOpt('v')->count, 3);
	}

	{
		commandParser.clear();
		commandParser.processString("test2");
		commandParser.processLine();

		CommandParsingState *cps = commandParser.getParsingState();
		assertNotNull(cps);
		assertEqualInt(cps->getParseSuccess(), false);
		assertEqualString(cps->getError(), "missing required option --coord (-c)");
	}

	{
		commandParser.clear();
		commandParser.processString("test2 -c 123 456");
		commandParser.processLine();

		CommandParsingState *cps = commandParser.getParsingState();
		assertNotNull(cps);
		assertEqualInt(cps->getParseSuccess(), true);
		
		CommandOptionParsingState *cops = cps->getByShortOpt('c');
		assertNotNull(cops);
		assertEqualInt(cops->getNumArgs(), 2);
		assertEqualInt(cops->getArgInt(0), 123);
		assertEqualInt(cops->getArgInt(1), 456);
	}

	{
		commandParser.clear();
		commandParser.processString("test3 -x 5 -lRvv --verbose");
		commandParser.processLine();

		CommandParsingState *cps = commandParser.getParsingState();
		assertNotNull(cps);
		assertEqualInt(cps->getParseSuccess(), true);
		
		CommandOptionParsingState *cops = cps->getByShortOpt('x');
		assertNotNull(cops);
		assertEqualInt(cops->getNumArgs(), 1);
		assertEqualInt(cops->getArgInt(0), 5);

		assertNotNull(cps->getByShortOpt('l'));	
		assertEqualInt(cps->getByShortOpt('l')->count, 1);

		assertNotNull(cps->getByShortOpt('R'));	
		assertEqualInt(cps->getByShortOpt('R')->count, 1);

		assertNotNull(cps->getByShortOpt('v'));	
		assertEqualInt(cps->getByShortOpt('v')->count, 3);

	}

	{
		commandParser.clear();
		commandParser.processString("test3 -x -l");
		commandParser.processLine();

		CommandParsingState *cps = commandParser.getParsingState();
		assertNotNull(cps);
		assertEqualInt(cps->getParseSuccess(), false);
		//Log.info(cps->getError());
		assertEqualString(cps->getError(), "missing required arguments to --x-value (-x)");
	}

	{
		commandParser.clear();
		commandParser.processString("test3 -l -x");
		commandParser.processLine();

		CommandParsingState *cps = commandParser.getParsingState();
		assertNotNull(cps);
		assertEqualInt(cps->getParseSuccess(), false);
		//Log.info(cps->getError());
		assertEqualString(cps->getError(), "missing required arguments to --x-value (-x)");
	}

	{
		commandParser.clear();
		commandParser.processString("test3 -z");
		commandParser.processLine();

		CommandParsingState *cps = commandParser.getParsingState();
		assertNotNull(cps);
		assertEqualInt(cps->getParseSuccess(), false);
		//Log.info(cps->getError());
		assertEqualString(cps->getError(), "unknown option -z");
	}

	Log.info("runUnitTest completed!");
}

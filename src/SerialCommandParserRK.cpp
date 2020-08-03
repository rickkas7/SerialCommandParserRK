#include "SerialCommandParserRK.h"


#include <string.h> // strtok_s

// Define the debug logging level here
// 0 = Off
// 1 = Normal
// 2 = High
#ifndef UNITTEST
	#define SERIAL_COMMAND_DEBUG_LEVEL 1
#else
	// No debug logging in unit test mode
	#define SERIAL_COMMAND_DEBUG_LEVEL 0
#endif

// Don't change these, just change the debugging level above
#if SERIAL_COMMAND_DEBUG_LEVEL >= 1
static Logger log("app.sercmd");

#define DEBUG_NORMAL(x) log.info x
#else
#define DEBUG_NORMAL(x)
#endif

#if SERIAL_COMMAND_DEBUG_LEVEL >= 2
#define DEBUG_HIGH(x) log.info x
#else
#define DEBUG_HIGH(x)
#endif


CommandOption::CommandOption(char shortOpt, const char *longOpt, const char *help, bool required, size_t requiredArgs) :
	shortOpt(shortOpt), longOpt(longOpt), help(help), required(required), requiredArgs(requiredArgs) {

}

CommandOption::~CommandOption() {
}	

String CommandOption::getName() const {
	if (longOpt && *longOpt) {
		// Has long option
		
		if (shortOpt > ' ') {
			// Has a readable short option so show both
			return String::format("--%s (-%c)", longOpt, shortOpt);
		}
		else {
			// Has a secret internal short option, only show long
			return String::format("--%s", longOpt);
		}
	}
	else {
		// Only short option
		return String::format("-%c", shortOpt);
	}
}

CommandHandlerInfo::CommandHandlerInfo(std::vector<String> cmdNames, const char *helpStr, std::function<void(SerialCommandParserBase *parser)> handler) :
	cmdNames(cmdNames), helpStr(helpStr), handler(handler) {

}

CommandHandlerInfo::~CommandHandlerInfo() {
	while(!cmdOptions.empty()) {
		delete cmdOptions.back();
		cmdOptions.pop_back();
	}

}


CommandHandlerInfo &CommandHandlerInfo::addCommandOption(char shortOpt, const char *longOpt, const char *help, bool required, size_t requiredArgs) {
	// Object added to cmdOptions and delete from the destructor
	return addCommandOption(new CommandOption(shortOpt, longOpt, help, required, requiredArgs));
}

CommandHandlerInfo &CommandHandlerInfo::addCommandOption(CommandOption *opt) {
	cmdOptions.push_back(opt);
	return *this;
}

const CommandOption *CommandHandlerInfo::getByShortOpt(char shortOpt) const {
	for(const CommandOption *opt : cmdOptions) {
		if (opt->shortOpt == shortOpt) {
			return opt;
		}
	}
	return NULL;
}

const CommandOption *CommandHandlerInfo::getByLongOpt(const char *longOpt) const {
	for(const CommandOption *opt : cmdOptions) {
		if (opt->longOpt && strcmp(opt->longOpt, longOpt) == 0) {
			return opt;
		}
	}
	return NULL;
}



SerialCommandConfig::SerialCommandConfig() {

}

SerialCommandConfig::~SerialCommandConfig() {
	while(!commandHandlers.empty()) {
		delete commandHandlers.back();
		commandHandlers.pop_back();
	}
}

CommandHandlerInfo &SerialCommandConfig::addCommandHandler(const char *cmdNames, const char *helpStr, std::function<void(SerialCommandParserBase *parser)> handler) {

	std::vector<String> cmdNamesVector;

	if (strchr(cmdNames, '|')) {
		char *mutableCopy = strdup(cmdNames);
		if (mutableCopy) {
			char *nextToken;
			char *token;
			const char *delim = "|";
			token = strtok_r(mutableCopy, delim, &nextToken);
			while(token) {
				cmdNamesVector.push_back(token);
				token = strtok_r(NULL, delim, &nextToken);
			}

			free(mutableCopy);
		}
	}
	else {
		// Has no aliases
		cmdNamesVector.push_back(cmdNames);
	}

	CommandHandlerInfo *chi = new CommandHandlerInfo(cmdNamesVector, helpStr, handler);

	commandHandlers.push_back(chi);

	return *chi;
}

void SerialCommandConfig::addHelpCommand(const char *helpCommands) {
	addCommandHandler(helpCommands, "", [this](SerialCommandParserBase *parser) {
		parser->printHelp();
	});
}

CommandHandlerInfo *SerialCommandConfig::getCommandHandlerInfo(const char *cmd) const {

	if (!commandHandlers.empty()) {
		for(CommandHandlerInfo *chi : commandHandlers) {
			for(String cmdName : chi->cmdNames) {
				if (strcmp(cmdName, cmd) == 0) {
					return chi;
				}
			}
		}
	}

	return NULL;
}

CommandArgsParserBase::CommandArgsParserBase() {
}

CommandArgsParserBase::~CommandArgsParserBase() {
}

bool CommandArgsParserBase::getArgBool(size_t index) const {
	char c = getArgChar(index, '0');

	return (c == '1' || c == 'T' || c == 't' || c == 'Y' || c == 'y');
}

int CommandArgsParserBase::getArgInt(size_t index) const {
	return atoi(getArgString(index));
}

float CommandArgsParserBase::getArgFloat(size_t index) const {
	return atof(getArgString(index));
}

char CommandArgsParserBase::getArgChar(size_t index, char defaultValue) const {
	const char *s = getArgString(index);
	if (s[0]) {
		return s[0];
	}
	else {
		return defaultValue;
	}
}


CommandArgsParserVector::CommandArgsParserVector(std::vector<String> &vec) : vec(vec) {
}

CommandArgsParserVector::~CommandArgsParserVector() {
}

size_t CommandArgsParserVector::getArgCount() const {
	return vec.size();
}

/**
 * @brief Get a parsed argument by index
 *
 * @param index The argument to get (0 = first, 1 = second, ...)
 *
 * If the index is out of bounds (larger than the largest argument), an empty string is returned.
 */
const char *CommandArgsParserVector::getArgString(size_t index) const {
	if (index < getArgCount()) {
		return vec[index];
	}
	else {
		return "";
	}
}

CommandArgsParserArray::CommandArgsParserArray(char **argsBuffer, size_t *argsCountPtr) : argsBuffer(argsBuffer), argsCountPtr(argsCountPtr) {
}

CommandArgsParserArray::~CommandArgsParserArray() {
}

size_t CommandArgsParserArray::getArgCount() const {
	return *argsCountPtr;
}

 const char *CommandArgsParserArray::getArgString(size_t index) const {
	 if (index < getArgCount()) {
		 return argsBuffer[index];
	 }
	 else {
		 return "";
	 }
 }


CommandOptionParsingState::CommandOptionParsingState() :
	CommandArgsParserVector(args) {

}

CommandOptionParsingState::~CommandOptionParsingState() {
	
}

CommandParsingState::CommandParsingState(CommandHandlerInfo *chi) : 
	CommandArgsParserVector(extraArgs), chi(chi) {

}

CommandParsingState::~CommandParsingState() {
	for(CommandOptionParsingState *optState : options) {
		delete optState;
	}
	options.clear();
}

void CommandParsingState::clear() {
	options.clear();
	extraArgs.clear();
	parseSuccess = false;
	err = "";
}


void CommandParsingState::parse(const char * const *argsBuffer, size_t argsCount) {
	//
	clear();

	for(size_t ii = 1; ii < argsCount; ii++) {
		if (argsBuffer[ii][0] == '-') {
			const CommandOption *opt = NULL;

			if (argsBuffer[ii][1] == '-') {
				// Long option
				opt = chi->getByLongOpt(&argsBuffer[ii][2]);
			}
			else {
				// Short option 
				for(size_t jj = 1; argsBuffer[ii][jj]; jj++) {
					opt = chi->getByShortOpt(argsBuffer[ii][jj]);
					if (!argsBuffer[ii][jj + 1]) {
						// Handle this below in case the last option has optional args
						break;
					}
					if (!opt) {
						err = String::format("unknown grouped short option -%c", argsBuffer[ii][jj]);
						DEBUG_HIGH((err.c_str()));
						return;
					}
					// Grouped short options can't have args except for the last arg 
					// (and even then it's allowed, but weird)
					getOrCreateByShortOpt(argsBuffer[ii][jj]);
				}
			}

			if (opt) {
				if (opt->requiredArgs > 0) {
					// Do the required args exist?
					for(size_t jj = 0; jj < opt->requiredArgs; jj++) {
						if (((ii + jj + 1) >= argsCount) ||
						 	(argsBuffer[ii + jj + 1][0] == '-')) {
							err = String::format("missing required arguments to %s", opt->getName().c_str());
							DEBUG_HIGH((err.c_str()));
							return;							
						}
					}
					CommandOptionParsingState *cops = getOrCreateByShortOpt(opt->shortOpt);
					for(size_t jj = 0; jj < opt->requiredArgs; jj++) {
						cops->args.push_back(argsBuffer[ii + jj + 1]);
					}
					// Skip over required args
					ii += opt->requiredArgs; 
				}
				else {
					// No required args so add it
					getOrCreateByShortOpt(opt->shortOpt);
				}
			}
			else {
				err = String::format("unknown option %s", argsBuffer[ii]);
				DEBUG_HIGH((err.c_str()));
				return;
			}
		}
		else {
			extraArgs.push_back(argsBuffer[ii]);
		}
	}

	// Check for missing required arguments
	for(const CommandOption *opt : chi->cmdOptions) {
		if (opt->required) {
			if (!getByShortOpt(opt->shortOpt)) {
				err = String::format("missing required option %s", opt->getName().c_str());
				DEBUG_HIGH((err.c_str()));
				return;
			}
		}
	}

	parseSuccess = true;
}

CommandOptionParsingState *CommandParsingState::getByShortOpt(char shortOpt) {
	for(CommandOptionParsingState *optState : options) {
		if (optState->shortOpt == shortOpt) {
			return optState;
		}
	}
	return NULL;
}


CommandOptionParsingState *CommandParsingState::getOrCreateByShortOpt(char shortOpt, bool incrementCount) {
	CommandOptionParsingState *optState = getByShortOpt(shortOpt);
	if (!optState) {
		optState = new CommandOptionParsingState();
		if (optState) {
			optState->shortOpt = shortOpt;
			options.push_back(optState);
		}
	}
	if (optState && incrementCount) {
		optState->count++;
	}
	return optState;
}


SerialCommandParserBase::SerialCommandParserBase(char *buffer, size_t bufferSize, char **argsBuffer, size_t argsBufferSize) :
	CommandArgsParserArray(argsBuffer, &argsCount),
	buffer(buffer), bufferSize(bufferSize), argsBuffer(argsBuffer), argsBufferSize(argsBufferSize) {

}

SerialCommandParserBase::~SerialCommandParserBase() {
	if (parsingState) {
		delete parsingState;
		parsingState = NULL;
	}
}

void SerialCommandParserBase::setup() {

}


void SerialCommandParserBase::loop() {
#ifndef UNITTEST
	if (stream) {
		if (streamType == StreamType::USBSerial) {
			USBSerial *usbSerial = (USBSerial *)stream;

			bool usbIsConnected = usbSerial->isConnected();
			if (usbIsConnected != usbWasConnected) {
				handleConnected(usbIsConnected);
				usbWasConnected = usbIsConnected;
			}
		}
		while(stream->available()) {
			filterChar(stream->read());
		}
	}
#endif /* UNITTEST */
}


void SerialCommandParserBase::clear() {
	bufferOffset = 0;
	argsCount = 0;
}


void SerialCommandParserBase::processString(const char *str) {
	for(size_t ii = 0; str[ii]; ii++) {
		processChar(str[ii]);
	}
	buffer[bufferOffset] = 0;
}


void SerialCommandParserBase::filterChar(char c) {
	processChar(c);
}

void SerialCommandParserBase::processChar(char c) {
	if (c == '\r' || c == '\n') {
		// End of command line

		if (bufferOffset > 0) {
			buffer[bufferOffset] = 0;

			processLine();

			// Empty buffer after processing
			clear();
		}
		return;
	}

	appendCharacter(c);
}

void SerialCommandParserBase::processLine() {
	// Process the line in buffer. The buffer is null terminated.

	if (handleRawLine()) {
		// Override is intercepting data
		return;
	}

	// Split into space-separated tokens, taking into account backslash escapes, single, and double-quoting
	char *src = buffer;

	for(argsCount = 0; argsCount < argsBufferSize; ) {
		// Skip leading white space. This does not need to check for backslash escapes.
		while(*src == ' ' || *src == '\t') {
			src++;
		}
		if (!*src) {
			break;
		}

		argsBuffer[argsCount++] = src;

		char *dst = src;

		bool inBackslash = false;
		bool inDoubleQuote = false;
		bool inSingleQuote = false;
		while(*src) {
			if (!inBackslash && *src == '\\') {
				// Backslash escape the next character regardless of double quote or single quote
				inBackslash = true;
				src++;
			}
			else {
				inBackslash = false;
			}

			if (inDoubleQuote) {
				if (*src == '"') {
					inDoubleQuote = false;
					src++;
				}
			}
			else
			if (inSingleQuote) {
				if (*src == '\'') {
					inSingleQuote = false;
					src++;
				}
			}

			if (!inDoubleQuote && !inSingleQuote && !inBackslash) {
				if (*src == '"') {
					inDoubleQuote = true;
					src++;
				}
				else
				if (*src == '\'') {
					inSingleQuote = true;
					src++;
				}
				else
				if (*src == ' ' || *src == '\t') {
					// Whitespace not in quotes or backslash marks the end of this argument
					break;
				}
			}

			// Copy character
			*dst++ = *src++;
		}

		bool atEnd = (*src == 0);

		*dst = 0;

		if (atEnd) {
			break;
		}

		src++;
	}

	// argsBuffer contains space-separated tokens, one per argument, with
	// leading and trailing spaces removed

	if (handleTokens()) {
		// Override is handling tokens itself
		return;
	}

	CommandHandlerInfo *chi = config->getCommandHandlerInfo(getArgString(0));
	if (chi) {
		if (parsingState) {
			delete parsingState;
			parsingState = NULL;
		}

		if (chi->hasOptions()) {
			parsingState = new CommandParsingState(chi);
			if (parsingState) {
				parsingState->parse(argsBuffer, argsCount);
				if (parsingState->getParseSuccess()) {
					// Success
					chi->handler(this);
				}
			}
		}
		else {
			// No options, call handler always
			chi->handler(this);
		}

	}
	else {
		DEBUG_HIGH(("unknown command '%s'", getArgString(0)));
		printHelp();
	}

	handlePrompt();
}

// Virtual override class Print
size_t SerialCommandParserBase::write(uint8_t c) {
#ifndef UNITTEST
	if (stream) {
		return stream->write(c);
	}
	else {
		return 0;
	}
#else
	putchar(c);
	return 1;
#endif /* UNITTEST */
}

void SerialCommandParserBase::printHelp() {
	for(CommandHandlerInfo *chi : config->getCommandHandlers()) {
		String additional;
		if (chi->cmdNames.size() > 1) {
			additional += "(";
			for(size_t ii = 1; ii < chi->cmdNames.size(); ii++) {
				additional += chi->cmdNames[ii];
				if ((ii + 1) < chi->cmdNames.size()) {
					additional += ", ";
				}
			}
			additional += ")";
		}

		printlnf("%s %s %s", chi->cmdNames[0].c_str(), chi->helpStr, additional.c_str());
	}
}


void SerialCommandParserBase::handlePrompt() {
	if (config->getPrompt().length() > 0) {
		print(config->getPrompt().c_str());
	}
}

void SerialCommandParserBase::handleWelcome() {
	if (config->getWelcome().length() > 0) {
		printWithNewLine(config->getWelcome(), true);
	}
}


size_t SerialCommandParserBase::printWithNewLine(const char *str, bool endWithNewLine) {
	const char *cp = str;
	char last = 0;
	size_t numLines = 0;

	while(*cp) {
		if (*cp == '\n' && last != '\r') {
			print('\r');
		}
		if (*cp == '\n') {
			numLines++;
		}
		print(*cp);
		last = *cp++;
	}
	if (endWithNewLine && last != '\n') {
		println("");
		numLines++;
	}
	return numLines;
}

bool SerialCommandParserBase::handleRawLine() {
	// false = continue with default behavior
	// true = don't parse into tokens, just clear buffer
	return false;
}

bool SerialCommandParserBase::handleTokens() {
	// false = continue with default behavior
	// true = don't use command handler
	return false;
}

void SerialCommandParserBase::handleConnected(bool usbIsConnected) {
	if (usbIsConnected) {
		handleWelcome();
		handlePrompt();
	}
}

void SerialCommandParserBase::deleteCharacterLeft(size_t index) {
	if (index == 0) {
		// Nothing to delete
		return;
	}
	if (index > bufferOffset) {
		index = bufferOffset;
	}
	if (index == bufferOffset) {
		// Delete at end
		bufferOffset--;
		return;
	}
	memmove(&buffer[index - 1], &buffer[index], bufferOffset - index);
	bufferOffset--;
}

void SerialCommandParserBase::deleteCharacterAt(size_t index) {
	if (index >= bufferOffset) {
		// Nothing to delete
		return;
	}

	memmove(&buffer[index], &buffer[index + 1], bufferOffset - index - 1);
	bufferOffset--;
}

void SerialCommandParserBase::deleteToEnd(size_t index) {
	if (index < bufferOffset) {
		bufferOffset = index;
	}
}


void SerialCommandParserBase::insertCharacterAt(size_t index, char c) {
	if (index > bufferOffset) {
		index = bufferOffset;
	}
	if (index >= bufferOffset && index < (bufferSize - 1)) {
		// Append
		buffer[bufferOffset++] = c;
		return;
	}

	// Insert
	memmove(&buffer[index + 1], &buffer[index], bufferOffset - index);
	buffer[index] = c;
	bufferOffset++;
}

void SerialCommandParserBase::appendCharacter(char c) {
	if (bufferOffset < (bufferSize - 1)) {
		buffer[bufferOffset++] = c;
	}
}

char *SerialCommandParserBase::getBuffer() {
	buffer[bufferOffset] = 0;
	return buffer;
}


SerialCommandEditorBase::SerialCommandEditorBase(char *historyBuffer, size_t historyBufferSize, char *buffer, size_t bufferSize, char **argsBuffer, size_t argsBufferSize) :
		SerialCommandParserBase(buffer, bufferSize, argsBuffer, argsBufferSize),
		historyBuffer(historyBuffer), historyBufferSize(historyBufferSize) {

	historyBuffer[0] = 0;

}

SerialCommandEditorBase::~SerialCommandEditorBase() {

}

/**
 * @brief Clear the data in the processor
 *
 * You normal don't need to call this as it's cleared after processing a command. This is
 * used during unit testing.
 */
void SerialCommandEditorBase::clear() {
	SerialCommandParserBase::clear();
	cursorPos = 0;
	horizScroll = 0;
	promptRendered = false;
	historyClear();
}




void SerialCommandEditorBase::handleConnected(bool isConnected) {
	clear();

	if (terminalType != TerminalType::DUMB) {
		getScreenSize();
	}
	else {
		startEditing();
	}
}

void SerialCommandEditorBase::getCursorPosition(std::function<void(int row, int col)> callback) {
	positionCallback = callback;
	printTerminalOutputSequence(6, 'n');
};

void SerialCommandEditorBase::setCursorPosition(int row, int col) {
	printf("%c[%d;%dH", KEY_ESC, row, col);
}

void SerialCommandEditorBase::printTerminalOutputSequence(int n, char c) {
	printf("%c[%d%c", KEY_ESC, n, c);
}

void SerialCommandEditorBase::getScreenSize() {
	// Get screen size:
	// https://stackoverflow.com/questions/35688348/how-do-i-determine-size-of-ansi-terminal
	gettingScreenSize = true;
	startScreenSizeMillis = millis();
	setCursorPosition(999, 999);
	getCursorPosition();
}



void SerialCommandEditorBase::loop() {
	// Check for escape
	if ((keyEscapeOffset == 1) && (millis() - lastKeyMillis > 10)) {
		// Got an ESC but did not get a [ right away, so it's probably someone hitting the ESC key
		DEBUG_HIGH(("esc timed out"));
		handleSpecialKey(KEY_ESC);
		keyEscapeOffset = 0;
	}
	if (startScreenSizeMillis != 0 && millis() - startScreenSizeMillis > 500) {
		// Terminal did not respond with a screen size. Set at 80x24.
		DEBUG_HIGH(("didn't get screen size"));
		startScreenSizeMillis = 0;
		terminalType = TerminalType::DUMB;
		startEditing();
	}

	// Call base class
	SerialCommandParserBase::loop();
}


void SerialCommandEditorBase::filterChar(char c) {
	// Log.trace("char %c %d", c, c);
	lastKeyMillis = millis();

	promptRendered = false;

	if (c == KEY_ESC && keyEscapeOffset == 0) {
		keyEscapeBuf[keyEscapeOffset++] = c;
	}
	else
	if (c < 32 || c == KEY_DELETE) {
		// Handle all control characters, also things like KEY_BACKSPACE, KEY_TAB, etc.
		handleSpecialKey(c);
		// We handle KEY_CR and KEY_LF out of handleSpecialKey
	}
	else
	if (keyEscapeOffset > 0) {
		// ("char %c", c);

		switch(keyEscapeOffset) {
		case 1:
			if (c == '[') {
				keyEscapeBuf[keyEscapeOffset++] = c;
			}
			else {
				// Send the ESC and also the key that was just pressed and clear ESC mode.
				DEBUG_HIGH(("esc not CSI"));
				handleSpecialKey(KEY_ESC);
				processChar(c);
				keyEscapeOffset = 0;
			}
			break;

		case 2:
			if (c >= 'A' && c <= 'Z') {
				// Single character xterm sequences
				DEBUG_HIGH(("got xterm  key %c", c));
				switch(c) {
				case 'A':
					handleSpecialKey(KEY_UP);
					break;

				case 'B':
					handleSpecialKey(KEY_DOWN);
					break;

				case 'C':
					handleSpecialKey(KEY_RIGHT);
					break;

				case 'D':
					handleSpecialKey(KEY_LEFT);
					break;
				}

				keyEscapeOffset = 0;
			}
			else
			if (c >= '0' && c <= '9') {
				keyEscapeBuf[keyEscapeOffset++] = c;
			}
			else {
				// Not a valid escape sequence
				keyEscapeOffset = 0;
			}
			break;

		default:
			if (keyEscapeOffset == 3) {
				if (keyEscapeBuf[2] >= '1' && keyEscapeBuf[2] < '9' && c >= 'A' && c <= 'Z') {
					// xterm with modifier or a function key. Not currently supported.
					DEBUG_HIGH(("got xterm function key %c", c));

					keyEscapeOffset = 0;
					break;
				}
			}
			if (keyEscapeOffset >= (sizeof(keyEscapeBuf) - 1) || c == '~' || c == 'R') {

				// End of sequence normally ends with ~ except for
				// DSR which returns ESC[n;mR, where n is the row and m is the column.
				keyEscapeBuf[keyEscapeOffset] = 0;

				char *semiColon = strchr(&keyEscapeBuf[2], ';');
				if (semiColon) {
					*semiColon = 0;
				}

				int n1, n2 = 0;

				n1 = atoi(&keyEscapeBuf[2]);
				if (semiColon) {
					n2 = atoi(&semiColon[1]);
				}
				if (c == 'R') {
					DEBUG_HIGH(("got DSR rows=%d cols=%d", n1, n2));
					if (gettingScreenSize) {
						gettingScreenSize = false;
						terminalType = TerminalType::ANSI;
						startScreenSizeMillis = 0;
						screenRows = n1;
						screenCols = n2;
						startEditing();
					}
					else {
						if (positionCallback) {
							positionCallback(n1, n2);
							positionCallback = 0;
						}
					}
				}
				else {
					DEBUG_HIGH(("got ansi n1=%d n2=%d", n1, n2));
					switch(n1) {
					case 1:
					case 7:
						handleSpecialKey(KEY_HOME);
						break;

					case 2:
					case 8:
						handleSpecialKey(KEY_INSERT);
						break;

					case 3:
						handleSpecialKey(KEY_FORWARD_DELETE);
						break;

					case 4:
						handleSpecialKey(KEY_END);
						break;

					case 5:
						handleSpecialKey(KEY_PAGE_UP);
						break;

					case 6:
						handleSpecialKey(KEY_PAGE_DOWN);
						break;

					default:
						break;
					}
				}
				keyEscapeOffset = 0;
			}
			else
			if ((c >= '0' && c <= '9') || c == ';') {
				keyEscapeBuf[keyEscapeOffset++] = c;
			}
			else {
				// Not a valid escape sequence
				keyEscapeOffset = 0;
			}
		}
	}
	else {
		processChar(c);
	}
}

void SerialCommandEditorBase::startEditing() {
	if (terminalType == TerminalType::ANSI) {
		eraseScreen();
		setCursorPosition(1, 1);
		handleWelcome();
	}
	handlePrompt();
}

void SerialCommandEditorBase::handlePrompt() {
	handlePromptWithCallback(0);
}

void SerialCommandEditorBase::handlePromptWithCallback(std::function<void()> handlePromptCallback) {
	if (promptRendered) {
		return;
	}
	promptRendered = true;

	// Call base class
	SerialCommandParserBase::handlePrompt();

	// Get the cursor position
	if (terminalType == TerminalType::ANSI) {
		getCursorPosition([this,handlePromptCallback](int row, int col) {
			editRow = row;
			editCol = col;

			DEBUG_HIGH(("prompt editRow=%d editCol=%d", editRow, editCol));

			eraseToEndOfLine();

			if (handlePromptCallback) {
				handlePromptCallback();
			}
		});
	}
	else {
		if (handlePromptCallback) {
			handlePromptCallback();
		}
	}
}

void SerialCommandEditorBase::handleCompletion() {
	// This null-terminates buffer
	char *cp = getBuffer();

	// Only do completion if we are at the end of the buffer and it has no spaces (command completion only for now)
	if (cursorPos != (int)bufferOffset || strchr(cp, ' ') != 0) {
		return;
	}

	// Looks like we can try completion

	std::vector<String> possibleMatches;

	for(CommandHandlerInfo *chi : config->getCommandHandlers()) {
		for(String s : chi->cmdNames) {
			if (strncmp(s, buffer, bufferOffset) == 0) {
				possibleMatches.push_back(s);
			}
		}
	}

	if (possibleMatches.size() == 0) {
		// No matches
		print(KEY_CTRL_G); // bell
		return;
	}
	else
	if (possibleMatches.size() == 1) {
		// Exactly one match, match the whole thing
		setBuffer(possibleMatches[0], true);
	}
	else {
		// Otherwise, find the longest match and only fill that much
		for(size_t ii = bufferOffset; ; ii++) {
			bool matchedAll = true;
			String s1;
			for(String s : possibleMatches) {
				if (s1.length() == 0) {
					s1 = s;
				}
				else {
					if (strncmp(s1, s, ii) != 0) {
						DEBUG_HIGH(("did not match s1=%s s=%s", s1.c_str(), s.c_str()));
						matchedAll = false;
						break;
					}
				}
			}
			if (!matchedAll) {
				// Only match up to ii - 1 characters
				setBuffer(s1.substring(0, ii - 1), true);
				DEBUG_HIGH(("matching up to %u: %s", ii - 1, buffer));
				print(KEY_CTRL_G); // bell
				break;
			}
		}
	}
}



void SerialCommandEditorBase::handleSpecialKey(char key) {
	if (terminalType == TerminalType::UNKNOWN) {
		if ((key == KEY_CR || key == KEY_LF || key == KEY_CTRL_L) && bufferOffset == 0 && screenRows == 0 && screenCols == 0) {
			// Hitting return with an unknown terminal type starts detection
			DEBUG_HIGH(("trying screen size"));
			getScreenSize();
		}
	}
	if (terminalType != TerminalType::ANSI) {
		SerialCommandParserBase::processChar(key);
		return;
	}

	DEBUG_HIGH(("special key %d", key));
	switch(key) {

	case KEY_CTRL_A:
	case KEY_HOME:
		cursorPos = 0;
		horizScroll = 0;
		redraw(0);
		setCursor();
		break;

	case KEY_BACKSPACE:
	case KEY_DELETE:
		// Delete the character to the left of cursorPos
		if (cursorPos > 0) {
			deleteCharacterLeft(cursorPos);
			cursorPos--;
			scrollToView(ScrollView::VISIBLE, true);
		}
		break;

	case KEY_CTRL_B:
	case KEY_LEFT:
		if (cursorPos > 0) {
			cursorPos--;
			if (cursorPos >= horizScroll) {
				cursorBack(1);
			}
			else {
				horizScroll = cursorPos;
				redraw(cursorPos);
				setCursor();
			}
		}
		break;

	case KEY_CTRL_E:
	case KEY_END:
		cursorPos = bufferOffset;
		scrollToView(ScrollView::END, true);
		break;

	case KEY_CTRL_F:
	case KEY_RIGHT:
		if (cursorPos < (int)bufferOffset) {
			cursorForward(1);
			cursorPos++;
			scrollToView(ScrollView::VISIBLE, false);
		}
		break;

	case KEY_TAB:
		handleCompletion();
		break;

	case KEY_CTRL_L:
		setCursorPosition(1, 1);
		eraseScreen();
		handlePromptWithCallback([this]() {
			redraw(horizScroll);
		});
		break;

	case KEY_CTRL_K:
		deleteToEnd(cursorPos);
		scrollToView(ScrollView::VISIBLE, true);
		break;

	case KEY_CTRL_N:
	case KEY_DOWN:
		if (curHistory > 0) {
			// There is a next history
			setBuffer(historyGet(--curHistory));
			scrollToView(ScrollView::END, false);

			if (curHistory == 0 && firstHistoryIsTemporary) {
				historyRemoveFirst();
				firstHistoryIsTemporary = false;
				curHistory = -1;
			}
		}
		else
		if (curHistory == 0) {
			// Down from first history if not temporary should make a blank line
			setBuffer("");
			curHistory = -1;
		}
		break;

	case KEY_CTRL_P:
	case KEY_UP:
		// Previous in history
		if ((curHistory + 1) < historySize()) {
			// There is an entry to go back to
			if (curHistory == -1 && bufferOffset > 0) {
				// Save existing buffer as temporary entry
				historyAdd(getBuffer(), true);
				curHistory++;
			}
			setBuffer(historyGet(++curHistory));
			scrollToView(ScrollView::END, false);
		}
		break;

	case KEY_CR:
	case KEY_LF:
		// Terminate the buffer and move to the next line
		buffer[bufferOffset] = 0;
		println("");
		if (editRow < screenRows) {
			editRow++;
		}

		// Save the line in history
		historyAdd(buffer, false);

		// processLine will redraw the prompt as well
		processLine();

		// Empty buffer after processing
		clear();

		break;

	case KEY_FORWARD_DELETE:
		if (cursorPos < (int)(bufferOffset - 1)) {
			deleteCharacterAt(cursorPos);
			scrollToView(ScrollView::VISIBLE, false);
		}
		break;

	}

}

void SerialCommandEditorBase::setBuffer(const char *str, bool atEnd) {
	size_t len = strlen(str);
	if (len < (bufferSize - 1)) {
		strcpy(buffer, str);
		bufferOffset = len;
	}
	else {
		strncpy(buffer, str, bufferSize - 1);
		buffer[bufferSize - 1] = 0;
		bufferOffset = bufferSize - 1;
	}
	if (atEnd) {
		scrollToView(ScrollView::END, true);
	}
	else {
		scrollToView(ScrollView::HOME, true);
	}
}

void SerialCommandEditorBase::processChar(char c) {
	if (terminalType != TerminalType::ANSI) {
		SerialCommandParserBase::processChar(c);
	}
	else
	if (cursorPos == (int)bufferOffset) {
		// Typing at end of the line
		appendCharacter(c);
		DEBUG_HIGH(("append %c at %d", c, cursorPos));

		int cursorCol = editCol + (cursorPos - horizScroll);
		if (cursorCol < (screenCols - 1)) {
			DEBUG_HIGH(("append %c at cursorPos=%d", c, cursorPos));
			print(c);
			cursorPos++;
		}
		else {
			// We're at the rightmost column so we need to scroll instead of just printing and wrapping
			cursorPos++;
			horizScroll++;
			DEBUG_HIGH(("append %c at cursorPos=%d with scroll horizScroll=%d", c, cursorPos, horizScroll));
			redraw(horizScroll);
		}
	}
	else {
		// Inserting in the middle of the line
		DEBUG_HIGH(("insert %c at cursorPos=%d bufferOffset=%d", c, cursorPos, bufferOffset));
		insertCharacterAt(cursorPos, c);
		redraw(cursorPos++);
	}
}

void SerialCommandEditorBase::scrollToView(ScrollView which, bool forceRedraw) {
	if (terminalType != TerminalType::ANSI) {
		return;
	}

	DEBUG_HIGH(("scrollToView which=%d forceRedraw=%d editCol=%d", which, forceRedraw, editCol));

	int widthRightOfPrompt = screenCols - editCol;

	switch(which) {
	case ScrollView::HOME:
		cursorPos = 0;
		horizScroll = 0;
		redraw(0);
		DEBUG_HIGH(("scrollToView which=HOME cursorPos=%d bufferOffset=%u horizScroll=%d", cursorPos, bufferOffset, horizScroll));
		break;

	case ScrollView::LEFT_EDGE:
		horizScroll = cursorPos;
		if ((int)bufferOffset <= widthRightOfPrompt) {
			horizScroll = 0;
		}

		if ((horizScroll + widthRightOfPrompt) > (int)bufferOffset) {
			horizScroll = bufferOffset - widthRightOfPrompt;
			redraw(horizScroll);
		}
		else
		if (forceRedraw) {
			redraw(horizScroll);
		}
		DEBUG_HIGH(("scrollToView which=LEFT_EDGE cursorPos=%d bufferOffset=%u horizScroll=%d forceRedraw=%d", cursorPos, bufferOffset, horizScroll, forceRedraw));
		break;

	case ScrollView::VISIBLE:
		if ((int)bufferOffset <= widthRightOfPrompt) {
			horizScroll = 0;
			DEBUG_HIGH(("scrollToView which=VISIBLE fits without scrolling bufferOffset=%u", bufferOffset));
		}

		if (cursorPos < horizScroll) {
			// Cursor left of scroll position
			scrollToView(ScrollView::LEFT_EDGE, forceRedraw);
		}
		else
		if (cursorPos > (horizScroll + widthRightOfPrompt)) {
			// Cursor to the right of the right edge
			scrollToView(ScrollView::RIGHT_EDGE, forceRedraw);
		}
		else
		if ((horizScroll + widthRightOfPrompt) > (int)bufferOffset) {
			// Scroll is too large for the current buffer (too much whitespace on right)
			horizScroll = bufferOffset - widthRightOfPrompt;
			if (horizScroll < 0) {
				horizScroll = 0;
			}
			redraw(horizScroll);
			DEBUG_HIGH(("scrollToView which=VISIBLE cursorPos=%d bufferOffset=%u horizScroll=%d", cursorPos, bufferOffset, horizScroll));
		}
		else
		if (forceRedraw) {
			redraw(horizScroll);
			DEBUG_HIGH(("scrollToView which=VISIBLE cursorPos=%d bufferOffset=%u horizScroll=%d forceRedraw=%d", cursorPos, bufferOffset, horizScroll, forceRedraw));
		}
		break;


	case ScrollView::RIGHT_EDGE:
		// We want cursorPos to be at the right edge of the display, unless
		// it would result in negative scrolling
		horizScroll = cursorPos - widthRightOfPrompt;
		if (horizScroll < 0) {
			horizScroll = 0;
			redraw(0);
		}
		else
		if (forceRedraw) {
			redraw(horizScroll);
		}
		DEBUG_HIGH(("scrollToView which=RIGHT_EDGE cursorPos=%d bufferOffset=%u horizScroll=%d forceRedraw=%d", cursorPos, bufferOffset, horizScroll, forceRedraw));
		break;

	case ScrollView::END:
		cursorPos = (int)bufferOffset;
		horizScroll = (int)bufferOffset - widthRightOfPrompt;
		if (horizScroll < 0) {
			horizScroll = 0;
		}
		redraw(horizScroll);
		DEBUG_HIGH(("scrollToView which=END cursorPos=%d bufferOffset=%u horizScroll=%d", cursorPos, bufferOffset, horizScroll));
		break;
	}
	setCursor();
}



void SerialCommandEditorBase::redraw(int fromPos) {
	// "fromPos" is the position in buffer to start drawing from
	// "editRow" and "editCol" are the cursor position right after the prompt (1, 1 = upper left corner)
	// "horizScroll" is the number of characters into buffer that we're scrolled. The character at
	// 	horizScroll is placed at editCol when scrolled. (Scrolling is done to the right of the prompt
	//	to avoid redrawing the prompt.)
	// "cursorPos" is the cursor position relative to buffer

	DEBUG_HIGH(("redraw fromPos=%d horizScroll=%d", fromPos, horizScroll));

	// "fromPosCol" is the column for fromPos (taking into account scrolling and the prompt)
	int fromPosCol = editCol + (fromPos - horizScroll);

	setCursorPosition(editRow, fromPosCol);
	if (fromPos < (int)bufferOffset) {
		char *cp = &getBuffer()[fromPos];

		int numToDraw = screenCols - fromPosCol - 1;
		if (numToDraw > ((int)bufferOffset - fromPos)) {
			numToDraw = ((int)bufferOffset - fromPos);
		}

		char *end = &getBuffer()[fromPos + numToDraw];
		while(cp < end) {
			print(*cp++);
		}
	}
	eraseToEndOfLine();
}

void SerialCommandEditorBase::setCursor() {
	DEBUG_HIGH(("setCursor editRow=%d editCol=%d cursorPos=%d horizScroll=%d", editRow, editCol, cursorPos, horizScroll));
	setCursorPosition(editRow, editCol + cursorPos - horizScroll);
}

void SerialCommandEditorBase::printMessage(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vprintMessage(true, fmt, ap);
	va_end(ap);
}

void SerialCommandEditorBase::printMessageNoPrompt(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vprintMessage(false, fmt, ap);
	va_end(ap);
}

void SerialCommandEditorBase::vprintMessage(bool prompt, const char *fmt, va_list ap) {
	char internalBuf[100], *message;

    size_t count = vsnprintf(internalBuf, sizeof(internalBuf), fmt, ap);

    if (count >= sizeof(internalBuf)) {
    	// Data is too large to fit in the static buffer
    	message = (char *)malloc(count + 1);
    	if (message) {
    		vsnprintf(message, count + 1, fmt, ap);
    	}
    	else {
    		// Just use truncated buffer if out of memory
    		message = internalBuf;
    		internalBuf[sizeof(internalBuf) - 1] = 0;
    	}
    }
    else {
    	message = internalBuf;
    }

	if (terminalType != TerminalType::ANSI) {
		// Just print the message for dumb terminal or non-interactive mode
		printWithNewLine(message, true);
	}
	else {
		// Move cursor to the left
		setCursorPosition(editRow, 1);
		eraseToEndOfLine();
		promptRendered = false;

		editRow += printWithNewLine(message, true);

		if (prompt) {
			printMessagePrompt();
		}
	}

    if (message != internalBuf) {
    	free((void *)message);
    }
}

void SerialCommandEditorBase::printMessagePrompt() {
	// Redraw previously entered line below he message
	handlePromptWithCallback([this]() {

		redraw(horizScroll);
		setCursor();
	});
}


void SerialCommandEditorBase::historyAdd(const char *line, bool temporary) {
	size_t len = strlen(line) + 1;

	if (len > historyBufferSize) {
		// Line is too long to fit, so ignore
		return;
	}

	if (firstHistoryIsTemporary) {
		// Remove the existing temporary history
		historyRemoveFirst();
	}

	while((strlen(historyBuffer) + len + 1) > historyBufferSize) {
		historyRemoveLast();
	}
	memmove(&historyBuffer[len], historyBuffer, strlen(historyBuffer) + 1);

	memmove(historyBuffer, line, len - 1);
	historyBuffer[len - 1] = '\n';

	firstHistoryIsTemporary = temporary;
}

String SerialCommandEditorBase::historyGet(int index) {

	char *cp, *last;

	cp = historyBuffer;
	for(size_t ii = 0; *cp; ii++) {
		last = cp;
		cp = strchr(last, '\n');
		if ((int)ii == index) {
			if (cp) {
				return String(last, cp - last);
			}
			else {
				return String(last);
			}
		}

		if (!cp) {
			break;
		}
		cp++;
	}
	return String("");
}

int SerialCommandEditorBase::historySize() {
	DEBUG_HIGH(("historySize: buffer=%s", historyBuffer));
	int size = 0;
	char *cp = historyBuffer;
	while(*cp) {
		size++;
		cp = strchr(cp, '\n');
		if (!cp) {
			break;
		}
		cp++;
	}
	return size;
}

void SerialCommandEditorBase::historyClear() {
	historyBuffer[0] = 0;
}

void SerialCommandEditorBase::historyRemoveFirst() {
	char *cp = strchr(historyBuffer, '\n');
	if (cp) {
		cp++;
		memmove(historyBuffer, cp,  strlen(cp) + 1);
	}
}

void SerialCommandEditorBase::historyRemoveLast() {
	// First remove the trailing \n
	size_t len = strlen(historyBuffer);
	if (len > 0 && historyBuffer[len - 1] == '\n') {
		historyBuffer[len - 1] = 0;
	}
	// Now find the \n before that
	char *cp = strrchr(historyBuffer, '\n');
	if (cp) {
		// And remove everything after it
		cp[1] = 0;
	}
	else {
		historyBuffer[0] = 0;
	}
}

#ifndef UNITTEST

SerialCommandTCPClient::SerialCommandTCPClient(SerialCommandTCPServer *server) : server(server) {
}

SerialCommandTCPClient::~SerialCommandTCPClient() {
	client.stop();

	if (editor) {
		delete editor;
	}
	delete[] historyBuffer;
	delete[] buffer;
	delete[] argsBuffer;

}

void SerialCommandTCPClient::setup() {
	historyBuffer = new char[server->historyBufSize];
	buffer = new char[server->bufferSize];
	argsBuffer = new char*[server->maxArgs];

	editor = new SerialCommandEditorBase(historyBuffer, server->historyBufSize, buffer, server->bufferSize, argsBuffer, server->maxArgs);
	if (editor) {
		editor->withConfig(server);
		editor->setup();
	}
}

void SerialCommandTCPClient::loop() {
	if (client.connected()) {
		editor->loop();
	}
	else {
		// Not connected
		if (wasConnected) {
			// Was previously connected, mark as not connected
			editor->handleConnected(false);
			wasConnected = false;
		}
	}
}

void SerialCommandTCPClient::setClient(TCPClient client) {
	this->client = client;
	editor->withStream(&this->client);
	editor->handleConnected(true);

	wasConnected = true;
};


SerialCommandTCPServer::SerialCommandTCPServer(size_t historyBufSize, size_t bufferSize, size_t maxArgs, size_t maxSessions, bool preallocate, uint16_t port) :
		historyBufSize(historyBufSize), bufferSize(bufferSize), maxArgs(maxArgs), maxSessions(maxSessions), preallocate(preallocate),
		server(port) {

}

SerialCommandTCPServer::~SerialCommandTCPServer() {

	if (clients) {
		for(size_t ii = 0; ii < maxSessions; ii++) {
			delete clients[ii];
		}
		delete[] clients;
	}
}

void SerialCommandTCPServer::setup() {
	clients = new SerialCommandTCPClient*[maxSessions];

	if (preallocate) {
		for(size_t ii = 0; ii < maxSessions; ii++) {
			clients[ii] = new SerialCommandTCPClient(this);

			clients[ii]->setup();

			if (!clients[ii]->isAllocated()) {
				DEBUG_NORMAL(("failed to allocate client %u, not enough RAM", ii));
				delete clients[ii];
				clients[ii] = 0;
			}
		}
	}
	else {
		for(size_t ii = 0; ii < maxSessions; ii++) {
			clients[ii] = 0;
		}
	}
}
void SerialCommandTCPServer::loop() {
	if (!clients) {
		return;
	}

	bool connected = isNetworkConnected();
	if (networkWasConnected != connected) {
		DEBUG_NORMAL(("networkConnected=%d", connected));
		if (connected) {
			// Network connected, initialize listener
#if Wiring_WiFi
			DEBUG_NORMAL(("IP address %s", WiFi.localIP().toString().c_str()));
#endif
			server.begin();
		}
		else {
			// Network disconnected, release all clients
			if (!preallocate) {
				for(size_t ii = 0; ii < maxSessions; ii++) {
					delete clients[ii];
					clients[ii] = 0;
				}
			}
		}
		networkWasConnected = connected;
	}

	for(size_t ii = 0; ii < maxSessions; ii++) {
		if (clients[ii]) {
			clients[ii]->loop();
			if (!preallocate) {
				if (!clients[ii]->isConnected()) {
					// Disconnected, free entry
					delete clients[ii];
					clients[ii] = 0;
					DEBUG_HIGH(("freed session=%u", ii));
				}
			}
		}
	}

	// Check for connections
	TCPClient client = server.available();
	if (client.connected()) {
		// Find a free entry
		bool foundFree = false;
		for(size_t ii = 0; ii < maxSessions; ii++) {
			if (preallocate) {
				if (clients[ii] && !clients[ii]->isConnected()) {
					clients[ii]->setClient(client);
					foundFree = true;
					DEBUG_HIGH(("connection started session=%u", ii));
					break;
				}
			}
			else {
				if (clients[ii] == 0) {
					clients[ii] = new SerialCommandTCPClient(this);
					clients[ii]->setup();
					if (clients[ii]->isAllocated()) {
						clients[ii]->setClient(client);
						foundFree = true;
						DEBUG_HIGH(("connection started session=%u", ii));
					}
					else {
						clients[ii]->stop();
						delete clients[ii];
						clients[ii] = 0;
						DEBUG_NORMAL(("failed to allocate client"));
					}
					break;
				}
			}
		}
		if (foundFree) {
			DEBUG_NORMAL(("connection from %s", client.remoteIP().toString().c_str()));
		}
		else {
			DEBUG_NORMAL(("connection from %s rejected, too many sessions", client.remoteIP().toString().c_str()));
			client.stop();
		}
	}
}

bool SerialCommandTCPServer::isNetworkConnected() {
#if Wiring_Cellular
	return Cellular.ready();
#endif
#if Wiring_WiFi
	return WiFi.ready();
#endif
#if Wiring_Ethernet
	return Ethernet.ready();
#endif
}

void SerialCommandTCPServer::stop(SerialCommandParserBase *parser) {
	for(size_t ii = 0; ii < maxSessions; ii++) {
		if (clients[ii]) {
			if (clients[ii]->getParser() == parser) {
				clients[ii]->stop();
				DEBUG_HIGH(("stop session=%u", ii));
				// Delete client from loop, not here
				break;
			}
		}
	}
}

#endif /* UNITTEST */

#ifndef UNITTEST

SerialCommandEditorLogHandlerBuffer::SerialCommandEditorLogHandlerBuffer(uint8_t *ringBuffer, size_t ringBufferSize, SerialCommandEditorBase *commandEditor, LogLevel level, LogCategoryFilters filters) :
	StreamLogHandler(*this, level, filters), ringBuffer(ringBuffer, ringBufferSize), commandEditor(commandEditor) {


}

SerialCommandEditorLogHandlerBuffer::~SerialCommandEditorLogHandlerBuffer() {

}


void SerialCommandEditorLogHandlerBuffer::setup() {
	// Add this handler into the system log manager
	LogManager::instance()->addHandler(this);
}


void SerialCommandEditorLogHandlerBuffer::loop() {

	while(true) {
		uint8_t c;

		bool bResult = ringBuffer.read(&c);
		if (!bResult) {
			break;
		}

		lineBuffer[lineBufferOffset++] = (char) c;
		if (c == '\n' || lineBufferOffset >= (sizeof(lineBuffer) - 1)) {
			lineBuffer[lineBufferOffset] = 0;

			if (ringBuffer.availableForRead() > 0) {
				commandEditor->printMessageNoPrompt("%s", lineBuffer);
			}
			else {
				commandEditor->printMessage("%s", lineBuffer);
			}
			lineBufferOffset = 0;
		}
	}
}

size_t SerialCommandEditorLogHandlerBuffer::write(uint8_t c) {

	return ringBuffer.write(&c) ? 1 : 0;
}

#endif /* UNITTEST */



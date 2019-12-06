#include "SerialCommandParserRK.h"

#include <string.h> // strtok_s

SerialCommandParserBase::SerialCommandParserBase(char *buffer, size_t bufferSize, char **argsBuffer, size_t argsBufferSize) :
	buffer(buffer), bufferSize(bufferSize), argsBuffer(argsBuffer), argsBufferSize(argsBufferSize) {

}

SerialCommandParserBase::~SerialCommandParserBase() {
	while(!commandHandlers.empty()) {
		delete commandHandlers.back();
		commandHandlers.pop_back();
	}
}

void SerialCommandParserBase::setup() {

}

void SerialCommandParserBase::clear() {
	bufferOffset = 0;
	argsCount = 0;
}


void SerialCommandParserBase::loop() {
#ifndef UNITTEST
	if (usartSerial) {
		while(usartSerial->available()) {
			filterChar(usartSerial->read());
		}
	}
	else
	if (usbSerial) {
		bool usbIsConnected = usbSerial->isConnected();
		if (usbIsConnected != usbWasConnected) {
			handleConnected(usbIsConnected);
			usbWasConnected = usbIsConnected;
		}

		while(usbSerial->available()) {
			filterChar(usbSerial->read());
		}
	}
#endif /* UNITTEST */
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

	bool handled = false;

	for(CommandHandlerInfo *chi : commandHandlers) {
		for(String cmdName : chi->cmdNames) {
			if (strcmp(cmdName, getArgString(0)) == 0) {
				chi->handler(this);
				handled = true;
				break;
			}
		}
	}
	if (!handled && !commandHandlers.empty()) {
		Log.info("unknown command '%s'", getArgString(0));
		printHelp();
	}
	handlePrompt();
}

// Virtual override class Print
size_t SerialCommandParserBase::write(uint8_t c) {
#ifndef UNITTEST
	if (usartSerial) {
		return usartSerial->write(c);
	}
	else
	if (usbSerial) {
		return usbSerial->write(c);
	}
	else {
		return 0;
	}
#else
	putchar(c);
	return 1;
#endif /* UNITTEST */
}

void SerialCommandParserBase::handlePrompt() {
	if (prompt.length() > 0) {
		print(prompt.c_str());
	}
}

void SerialCommandParserBase::handleWelcome() {
	if (welcome.length() > 0) {
		printWithNewLine(welcome, true);
	}
}


void SerialCommandParserBase::printWithNewLine(const char *str, bool endWithNewLine) {
	const char *cp = str;
	char last = 0;

	while(*cp) {
		if (*cp == '\n' && last != '\r') {
			print('\r');
		}
		print(*cp);
		last = *cp++;
	}
	if (endWithNewLine && last != '\n') {
		println("");
	}
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

const char *SerialCommandParserBase::getArgString(size_t index) const {
	if (index < argsCount) {
		return argsBuffer[index];
	}
	else {
		return "";
	}
}
bool SerialCommandParserBase::getArgBool(size_t index) const {
	const char *s = getArgString(index);
	switch(*s) {
	case '1':
	case 'Y':
	case 'y':
	case 'T':
	case 't':
		return true;

	default:
		return false;
	}
}

int SerialCommandParserBase::getArgInt(size_t index) const {
	return atoi(getArgString(index));
}

float SerialCommandParserBase::getArgFloat(size_t index) const {
	return atof(getArgString(index));
}

void SerialCommandParserBase::addCommandHandler(const char *cmdNames, const char *helpStr, std::function<void(SerialCommandParserBase *parser)> handler) {

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
}

void SerialCommandParserBase::addHelpCommand(const char *helpCommands) {
	addCommandHandler(helpCommands, "", [this](SerialCommandParserBase *) {
		printHelp();
	});
}

void SerialCommandParserBase::printHelp() {
	for(CommandHandlerInfo *chi : commandHandlers) {
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

		printlnf("%s %s %s", chi->cmdNames[0].c_str(), chi->helpStr.c_str(), additional.c_str());
	}
}

SerialCommandEditorBase::SerialCommandEditorBase(char *historyBuffer, size_t historyBufferSize, char *buffer, size_t bufferSize, char **argsBuffer, size_t argsBufferSize) :
		SerialCommandParserBase(buffer, bufferSize, argsBuffer, argsBufferSize),
		historyBuffer(historyBuffer), historyBufferSize(historyBufferSize) {

	historyBuffer[0] = 0;

}

SerialCommandEditorBase::~SerialCommandEditorBase() {

}

void SerialCommandEditorBase::handleConnected(bool isConnected) {
	getScreenSize();
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
		// Log.info("esc timed out");
		handleSpecialKey(KEY_ESC);
		keyEscapeOffset = 0;
	}
	if (startScreenSizeMillis != 0 && millis() - startScreenSizeMillis > 500) {
		// Terminal did not respond with a screen size. Set at 80x24.
		// Log.info("didn't get screen size");
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
				// Log.info("esc not CSI");
				handleSpecialKey(KEY_ESC);
				processChar(c);
				keyEscapeOffset = 0;
			}
			break;

		case 2:
			if (c >= 'A' && c <= 'Z') {
				// Single character xterm sequences
				// Log.info("got xterm  key %c", c);
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
					// Log.info("got xterm function key %c", c);

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
					// Log.info("got DSR rows=%d cols=%d", n1, n2);
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
					// Log.info("got ansi n1=%d n2=%d", n1, n2);
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
	// Call base class
	SerialCommandParserBase::handlePrompt();

	// Get the cursor position
	if (terminalType == TerminalType::ANSI) {
		getCursorPosition([this](int row, int col) {
			editRow = row;
			editCol = col;
			cursorPos = 0;
			horizScroll = 0;

			// Log.info("prompt editRow=%d editCol=%d", editRow, editCol);

			eraseToEndOfLine();

			// Testing: The cursor position is not changing on screen for some reason,
			// try explicitly setting it again
			setCursorPosition(row, col);
		});
	}
}

void SerialCommandEditorBase::handleSpecialKey(char key) {
	if (terminalType == TerminalType::UNKNOWN) {
		if ((key == KEY_CR || key == KEY_LF || key == KEY_CTRL_L) && bufferOffset == 0 && screenRows == 0 && screenCols == 0) {
			// Hitting return with an unknown terminal type starts detection
			// Log.info("trying screen size");
			getScreenSize();
		}
	}
	if (terminalType != TerminalType::ANSI) {
		SerialCommandParserBase::processChar(key);
		return;
	}

	// Log.info("special key %d", key);
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
			scrollToView(ScrollView::VISIBLE, false);
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

	case KEY_CTRL_L:
		setCursorPosition(1, 1);
		eraseScreen();
		handlePrompt();
		redraw(horizScroll);
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

		// Save the line in history
		historyAdd(buffer, false);

		// processLine will redraw the prompt as well
		processLine();

		// Empty buffer after processing
		clear();

		break;

	case KEY_FORWARD_DELETE:
		if (cursorPos < (bufferOffset - 1)) {
			deleteCharacterAt(cursorPos);
			scrollToView(ScrollView::VISIBLE, false);
		}
		break;

	}

}

void SerialCommandEditorBase::setBuffer(const char *str) {
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
	cursorPos = 0;
	horizScroll = 0;
	redraw(0);
	setCursor();
}

void SerialCommandEditorBase::processChar(char c) {
	if (terminalType != TerminalType::ANSI) {
		SerialCommandParserBase::processChar(c);
	}
	else
	if (cursorPos == (int)bufferOffset) {
		// Typing at end of the line
		appendCharacter(c);
		// Log.info("append %c at %d", c, cursorPos);

		int cursorCol = editCol + (cursorPos - horizScroll);
		if (cursorCol < (screenCols - 1)) {
			// Log.info("append %c at cursorPos=%d", c, cursorPos);
			print(c);
			cursorPos++;
		}
		else {
			// We're at the rightmost column so we need to scroll instead of just printing and wrapping
			cursorPos++;
			horizScroll++;
			// Log.info("append %c at cursorPos=%d with scroll horizScroll=%d", c, cursorPos, horizScroll);
			redraw(horizScroll);
		}
	}
	else {
		// Inserting in the middle of the line
		// Log.info("insert %c at cursorPos=%d bufferOffset=%d", c, cursorPos, bufferOffset);
		insertCharacterAt(cursorPos, c);
		redraw(cursorPos++);
	}
}

void SerialCommandEditorBase::scrollToView(ScrollView which, bool forceRedraw) {
	if (terminalType != TerminalType::ANSI) {
		return;
	}

	// Log.info("scrollToView which=%d forceRedraw=%d editCol=%d", which, forceRedraw, editCol);

	int widthRightOfPrompt = screenCols - editCol;

	if ((int)bufferOffset <= widthRightOfPrompt) {
		// Fits without scrolling
		horizScroll = 0;
		redraw(0);
		return;
	}

	switch(which) {
	case ScrollView::HOME:
		cursorPos = 0;
		horizScroll = 0;
		redraw(0);
		break;

	case ScrollView::LEFT_EDGE:
		horizScroll = cursorPos;
		if ((horizScroll + widthRightOfPrompt) > (int)bufferOffset) {
			horizScroll = bufferOffset - widthRightOfPrompt;
			redraw(horizScroll);
		}
		else
		if (forceRedraw) {
			redraw(horizScroll);
		}
		break;

	case ScrollView::VISIBLE:
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
			redraw(horizScroll);
		}
		else
		if (forceRedraw) {
			redraw(horizScroll);
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
		break;

	case ScrollView::END:
		cursorPos = (int)bufferOffset;
		horizScroll = (int)bufferOffset - widthRightOfPrompt;
		redraw(horizScroll);
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

	// Log.info("redraw fromPos=%d horizScroll=%d", fromPos, horizScroll);

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
	setCursorPosition(editRow, editCol + cursorPos - horizScroll);
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
	// Log.info("historySize: buffer=%s", historyBuffer);
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

// firstHistoryIsTemporary


// In the future, add a line editing mode

// https://en.wikipedia.org/wiki/GNU_Readline
// Ctrl-A Move cursor to start of line (Home)
// Ctrl-B Move cursor back (left arrow)
// Ctrl-E Move cursor to end of line (End)
// Ctrl-F Move cursor forward (right arrow)
// Ctrl-H Delete previous character (backspace)
// Ctrl-I Completion (tab)
// Ctrl-K Clear content after cursor
// Ctrl-L Clear screen
// Ctrl-N Next Command (down arrow)
// Ctrl-P Previous Command (up arrow)
// Ctrl-T Transpose characters (not currently supported)
//

// https://en.wikipedia.org/wiki/ANSI_escape_code


// Also add a command line option parser


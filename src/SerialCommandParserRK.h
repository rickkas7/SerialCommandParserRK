#ifndef __SERIALCOMMANDPARSERRK_H
#define __SERIALCOMMANDPARSERRK_H

#include "Particle.h"

#include <vector>

class SerialCommandParserBase; // Forward declaration

/**
 * @brief Class to hold information about a single command
 *
 * @param cmdNames vector of command names. First is the primary name, subsequent are aliases.
 *
 * @param helpStr the help string to display
 *
 * @param handler Function to call to handle the command
 *
 * You normally don't use this class directly; it's instantiated in addCommandHandler() for you.
 */
class CommandHandlerInfo {
public:
	CommandHandlerInfo(std::vector<String> cmdNames, const char *helpStr, std::function<void(SerialCommandParserBase *parser)> handler) :
		cmdNames(cmdNames), helpStr(helpStr), handler(handler) {};
	virtual ~CommandHandlerInfo() {};

	std::vector<String> cmdNames;
	String	helpStr;
	std::function<void(SerialCommandParserBase *parser)> handler;
};

/**
 * @brief Base class for serial command parsers.
 *
 * You may want to use SerialCommandParser (templated version) instead so the buffers can be allocated
 * statically on the heap at compile time and fewer parameters are involved.
 */
class SerialCommandParserBase : public Print {
public:
	/**
	 * @brief Constructor
	 *
	 * @param buffer Buffer to hold a single line. Must be one byte longer than the longest line you plan to parse
	 * because a null terminator will be added.
	 *
	 * @param bufferSize Size of the buffer in bytes.
	 *
	 * @param argsBuffer Array of char * pointers to hold the parsed argument pointers.
	 *
	 * @param argsBufferSize Number of entries in the argsBuffer array. For example, if you pass 10
	 * the you can parse up to 10 arguments. Additional arguments are ignored.
	 */
	SerialCommandParserBase(char *buffer, size_t bufferSize, char **argsBuffer, size_t argsBufferSize);

	/**
	 * @brief Destructor. Normally you instatiate one of these as a global variable so it won' be deleted.
	 */
	virtual ~SerialCommandParserBase();

	/**
	 * @brief Call from setup()
	 */
	void setup();

	/**
	 * @brief Call from loop()
	 *
	 * Serial is read and and callbacks are called from within loop.
	 */
	void loop();

	/**
	 * @brief Clear the data in the processor
	 *
	 * You normal don't need to call this as it's cleared after processing a command. This is
	 * used during unit testing.
	 */
	void clear();

	/**
	 * @brief Process a string of data
	 *
	 * @param str A c-string (null terminated) containing the data to parse.
	 *
	 * You normally won't use this; it's used by the unit tester. After saving the command data
	 * using processString(), call processLine() to process the line.
	 */
	void processString(const char *str);

	/**
	 * @brief Process a character
	 *
	 * Normally you don't call this. It's called from loop() when there is serial data available. It
	 * just calls processChar normally, but terminal editing support overrides this method.
	 */
	virtual void filterChar(char c);

	/**
	 * @brief Process a character
	 *
	 * Normally you don't call this. It's called from processChar() when there is serial data available.
	 */
	virtual void processChar(char c);

	/**
	 * @brief Process a completed line
	 *
	 * Called from processChar() when a <CR> or <LF> is encountered and there is a non-empty line
	 * in buffer.
	 */
	virtual void processLine();

	// Virtual override class Print
    virtual size_t write(uint8_t);

    /**
     * @brief Override to change the default behavior of generating a command prompt
     */
    virtual void handlePrompt();

    /**
     * @brief Prints the welcome message
     */
    virtual void handleWelcome();

    /**
     * @brief Override to handle the raw line before splitting into tokens
     *
     * @return true to override the default behavior or false to continue processing normally
     */
    virtual bool handleRawLine();

    /**
     * @brief Override to handle the tokens before using the automatic command processor
     *
     * @return true to override the default behavior or false to continue processing normally
     */
    virtual bool handleTokens();

    /**
     * @brief Override to handle connected/disconnected state change
     *
     * This is currently only supported for USB serial at this time as there is no way to know
     * if a device is connected to a hardware UART serial port at this time.
     */
    virtual void handleConnected(bool isConnected);


    void deleteCharacterLeft(size_t index);

    void deleteCharacterAt(size_t index);

    void deleteToEnd(size_t index);

    void insertCharacterAt(size_t index, char c);

    void appendCharacter(char c);


#ifndef UNITTEST
	/**
	 * @brief Sets the hardware serial port to read/write to
	 *
	 * This overload is used for hardware UARTs: Serial1, Serial2, Serial3, ...
	 */
	SerialCommandParserBase &withSerial(USARTSerial *serial) { this->usbSerial = 0; this->usartSerial = serial; return *this; };

	/**
	 * @brief Sets the USB serial port to read/write to
	 *
	 * This overload is used for USB serial ports: Serial, USBSerial1.
	 */
	SerialCommandParserBase &withSerial(USBSerial *serial) { this->usbSerial = serial; this->usartSerial = 0; return *this; };
#endif /* UNITTEST */

	SerialCommandParserBase &withPrompt(const char *prompt) { this->prompt = prompt; return *this; };

	/**
	 * @brief Set the welcome message upon connection (USB serial only) (optional)
	 *
	 * Since the hardware UART doesn't have connection detection, this is not used.
	 */
	SerialCommandParserBase &withWelcome(const char *welcome) { this->welcome = welcome; return *this; };

	/**
	 * @brief Print a string with lines terminated with \n expanded to \r\n
	 */
	void printWithNewLine(const char *str, bool endWithNewLine);

	char *getBuffer();

	/**
	 * @brief Gets the argument buffer directly
	 *
	 * Normally you'll want to use getArgString(), getArgInt(), etc.
	 */
	char **getArgsBuffer() { return argsBuffer; };

	/**
	 * @brief Get the number of arguments
	 *
	 * 0 = none, 1 = one argument, ... The index passed to getArgString(), getArgInt(),
	 */
	size_t getArgsCount() const { return argsCount; };

	/**
	 * @brief Get a parsed argument by index
	 *
	 * @param index The argument to get (0 = first, 1 = second, ...)
	 *
	 * If the index is out of bounds (larger than the largest argument), an empty string is returned.
	 */
	const char *getArgString(size_t index) const;

	/**
	 * @brief Get a parsed argument by as a bool
	 *
	 * @param index The argument to get (0 = first, 1 = second, ...)
	 *
	 * If the argument begins with 1, T, t, Y, or y, then true is returned. Any other string returns false.
	 *
	 * If the index is out of bounds (larger than the largest argument), false is returned.
	 */
	bool getArgBool(size_t index) const;

	/**
	 * @brief Get a parsed argument by as an int
	 *
	 * @param index The argument to get (0 = first, 1 = second, ...)
	 *
	 * If the value is not a number, then 0 is returned. It uses atoi internally, so rules for that apply.
	 *
	 * If the index is out of bounds (larger than the largest argument), 0 is returned.
	 */
	int getArgInt(size_t index) const;

	/**
	 * @brief Get a parsed argument by as a float
	 *
	 * @param index The argument to get (0 = first, 1 = second, ...)
	 *
	 * If the value is not a number, then 0 is returned. It uses atof internally, so rules for that apply.
	 *
	 * If the index is out of bounds (larger than the largest argument), 0 is returned.
	 */
	float getArgFloat(size_t index) const;

	/**
	 * @brief Add a command handler
	 *
	 * @param cmdName The name of the command, or a list of aliases separated by |. For example: "quit|exit" with no extra spaces.
	 *
	 * @param helpStr The help string to display for this command. This should not include the command name or alises; these
	 * are added automatically.
	 *
	 * @param handler The function or lambda to call when the command is entered.
	 */
	void addCommandHandler(const char *cmdName, const char *helpStr, std::function<void(SerialCommandParserBase *parser)> handler);

	/**
	 * @brief Add a help command. The default is "help" or "?" but you can override this.
	 *
	 * Typically you call this after adding all of your commands using addCommandHandler().
	 */
	void addHelpCommand(const char *helpCommand = "help|?");

	/**
	 * @brief Prints the help message.
	 *
	 * This is normally handled automatically by addHelpCommand(), but is separate so the help/usage can be displayed
	 * when you enter an invalid command as well.
	 */
	void printHelp();

protected:
	char *buffer;
	size_t bufferSize;
	char **argsBuffer;
	size_t argsBufferSize;
	size_t argsCount = 0;
	size_t bufferOffset = 0;
#ifndef UNITTEST
	USBSerial *usbSerial = 0;
	USARTSerial *usartSerial = 0;
	bool usbWasConnected = false;
#endif /* UNITTEST */
	std::vector<CommandHandlerInfo*> commandHandlers;
	String prompt;
	String welcome;
};


template<size_t BUFFER_SIZE, size_t MAX_ARGS>
class SerialCommandParser : public SerialCommandParserBase {
public:
	SerialCommandParser() : SerialCommandParserBase(staticBuffer, BUFFER_SIZE, staticArgsBuffer, MAX_ARGS) {};
	virtual ~SerialCommandParser() {};

protected:
	char staticBuffer[BUFFER_SIZE];
	char *staticArgsBuffer[MAX_ARGS];

};

class SerialCommandEditorBase : public SerialCommandParserBase {
public:
	enum class ScrollView {
		HOME,
		LEFT_EDGE,
		VISIBLE,
		RIGHT_EDGE,
		END
	};
	enum class TerminalType {
		UNKNOWN,
		DUMB,
		ANSI
	};


	SerialCommandEditorBase(char *historyBuffer, size_t historyBufferSize, char *buffer, size_t bufferSize, char **argsBuffer, size_t argsBufferSize);
	virtual ~SerialCommandEditorBase();


	void cursorUp(int n = 1) { printTerminalOutputSequence(n, 'A'); };

	void cursorDown(int n = 1) { printTerminalOutputSequence(n, 'B'); };

	void cursorForward(int n = 1) { printTerminalOutputSequence(n, 'C'); };

	void cursorBack(int n = 1) { printTerminalOutputSequence(n, 'D'); };

	void eraseScreen(int n = 2) { printTerminalOutputSequence(n, 'J'); };

	void eraseToBeginningOfScreen() { eraseScreen(1); };

	void eraseToEndOfScreen() { eraseScreen(0); };

	void eraseLine(int n = 2) { printTerminalOutputSequence(n, 'K'); };

	void eraseToBeginningOfLine() { eraseLine(1); };

	void eraseToEndOfLine() { eraseLine(0); };


	/**
	 * @brief Gets the cursor position using the Device Status Report (DSR)
	 */
	void getCursorPosition(std::function<void(int row, int col)> callback = 0);

	/**
	 * @brief Set cursor position (1 based, upper left is 1, 1)
	 */
	void setCursorPosition(int row, int col);

	void printTerminalOutputSequence(int n, char c);

	void getScreenSize();

	virtual void startEditing();

	void handleConnected(bool isConnected);

	void loop();

	void filterChar(char c);

	void handleSpecialKey(char key);

	virtual void handlePrompt();

	void setBuffer(const char *str);

	virtual void processChar(char c);

	void scrollToView(ScrollView which, bool forceRedraw);

	void redraw(int fromPos = 0);

	void setCursor();

	void historyAdd(const char *line, bool temporary = false);
	String historyGet(int index);
	int historySize();
	void historyClear();
	void historyRemoveFirst();
	void historyRemoveLast();

	static const char KEY_CTRL_A = 1;
	static const char KEY_CTRL_B = 2;
	static const char KEY_CTRL_C = 3;
	static const char KEY_CTRL_D = 4;
	static const char KEY_CTRL_E = 5;
	static const char KEY_CTRL_F = 6;
	static const char KEY_CTRL_G = 7;
	static const char KEY_CTRL_H = 8;
	static const char KEY_BACKSPACE = 8; // ^H Mac uses KEY_DELETE not KEY_BACKSPACE
	static const char KEY_CTRL_I = 9;
	static const char KEY_TAB = 9; // ^I
	static const char KEY_CTRL_J = 10;
	static const char KEY_LF = 10; // ^J
	static const char KEY_CTRL_K = 11;
	static const char KEY_CTRL_L = 12;
	static const char KEY_CTRL_M = 13;
	static const char KEY_CR = 13; // ^M
	static const char KEY_CTRL_N = 14;
	static const char KEY_CTRL_O = 15;
	static const char KEY_CTRL_P = 16;
	static const char KEY_CTRL_Q = 17;
	static const char KEY_CTRL_R = 18;
	static const char KEY_CTRL_S = 19;
	static const char KEY_CTRL_T = 20;
	static const char KEY_CTRL_U = 21;
	static const char KEY_CTRL_V = 22;
	static const char KEY_CTRL_W = 23;
	static const char KEY_CTRL_X = 24;
	static const char KEY_CTRL_Y = 25;
	static const char KEY_CTRL_Z = 26;
	static const char KEY_ESC = 27; // 0x1b
	static const char KEY_DELETE = 127; // Note: different than Forward Delete (

	static const char KEY_HOME = -1;
	static const char KEY_INSERT = -2;
	static const char KEY_FORWARD_DELETE = -3; // Key in the cursor keypad near Home, End, etc.
	static const char KEY_END = -4;
	static const char KEY_PAGE_UP = -5;
	static const char KEY_PAGE_DOWN = -6;
	static const char KEY_UP = -50;
	static const char KEY_DOWN = -51;
	static const char KEY_LEFT = -52;
	static const char KEY_RIGHT = -53;

protected:
	char *historyBuffer;
	size_t historyBufferSize;
	char keyEscapeBuf[10];
	size_t keyEscapeOffset = 0;
	bool gettingScreenSize = false;
	int screenRows = 0;
	int screenCols = 0;
	unsigned long lastKeyMillis = 0;
	unsigned long startScreenSizeMillis = 0;
	TerminalType terminalType = TerminalType::UNKNOWN;
	int editRow = 0;
	int editCol = 0;
	int cursorPos = 0;
	int horizScroll = 0;
	int curHistory = -1;
	bool firstHistoryIsTemporary = false;
	std::function<void(int row, int col)> positionCallback = 0;
};

template<size_t HISTORY_BUFFER_SIZE, size_t BUFFER_SIZE, size_t MAX_ARGS>
class SerialCommandEditor : public SerialCommandEditorBase {
public:
	SerialCommandEditor() : SerialCommandEditorBase(historyBuffer, HISTORY_BUFFER_SIZE, staticBuffer, BUFFER_SIZE, staticArgsBuffer, MAX_ARGS) {
		historyBuffer[0] = 0;
	};
	virtual ~SerialCommandEditor() {};


protected:
	char historyBuffer[HISTORY_BUFFER_SIZE];
	char staticBuffer[BUFFER_SIZE];
	char *staticArgsBuffer[MAX_ARGS];
};


#endif /* __SERIALCOMMANDPARSERRK_H */

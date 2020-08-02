#ifndef __SERIALCOMMANDPARSERRK_H
#define __SERIALCOMMANDPARSERRK_H

#include "Particle.h"
#include "RingBuffer.h"

#include <vector>

class SerialCommandParserBase; // Forward declaration

/**
 * @brief Specifies information about a single option for a command
 */
class CommandOption {
public:
	CommandOption();
	virtual ~CommandOption();	

	CommandOption(char shortOpt, const char *longOpt, const char *help, bool required = false, size_t requiredArgs = 0);

	String getName() const;

	// Configuration parameters
	char shortOpt = 0;
	const char *longOpt = NULL;
	const char *help = NULL;
	bool required = false;
	size_t requiredArgs = 0;
};


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
	CommandHandlerInfo(std::vector<String> cmdNames, const char *helpStr, std::function<void(SerialCommandParserBase *parser)> handler);
	virtual ~CommandHandlerInfo();

	CommandHandlerInfo &addCommandOption(char shortOpt, const char *longOpt, const char *help, bool required = false, size_t requiredArgs = 0);
	CommandHandlerInfo &addCommandOption(CommandOption *opt);

	const CommandOption *getByShortOpt(char shortOpt) const;

	const CommandOption *getByLongOpt(const char *longOpt) const;

	bool hasOptions() const { return !cmdOptions.empty(); };

	std::vector<String> cmdNames;
	const char *helpStr;
	std::vector<CommandOption*> cmdOptions;
	std::function<void(SerialCommandParserBase *parser)> handler;
};

/**
 * @brief Class that specifies a single option and possibly args
 */
class CommandOptionParsingState {
public:
	CommandOptionParsingState();
	virtual ~CommandOptionParsingState();

	size_t getNumArgs() const { return args.size(); };

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
	 * @brief Gets the first character of the argument
	 *
	 * @param index The argument to get (0 = first, 1 = second, ...)
	 *
	 * @param defaultValue if the argument index does not exist, return this value
	 */
	char getArgChar(size_t index, char defaultValue) const;

	char shortOpt = 0;
	size_t count = 0;
	std::vector<String> args;
};

/**
 * @brief Class that holds the result of parsing a command line with options
 */
class CommandParsingState {
public:
	CommandParsingState(CommandHandlerInfo *chi);
	virtual ~CommandParsingState();

	void clear();
	void parse(const char * const *argsBuffer, size_t argsCount);

	CommandOptionParsingState *getByShortOpt(char shortOpt);
	CommandOptionParsingState *getOrCreateByShortOpt(char shortOpt, bool incrementCount = true);

	size_t getNumExtraArgs() const { return extraArgs.size(); };
	bool getParseSuccess() const { return parseSuccess; };
	const char *getError() const { return err.c_str(); };

protected:
	CommandHandlerInfo *chi;
	std::vector<CommandOptionParsingState*> options;
	std::vector<String> extraArgs;
	bool parseSuccess = false;
	String err;
};


class SerialCommandConfig {
public:
	SerialCommandConfig();
	virtual ~SerialCommandConfig();

	SerialCommandConfig &withPrompt(const char *prompt) { this->prompt = prompt; return *this; };

	/**
	 * @brief Set the welcome message upon connection (USB serial only) (optional)
	 *
	 * Since the hardware UART doesn't have connection detection, this is not used.
	 */
	SerialCommandConfig &withWelcome(const char *welcome) { this->welcome = welcome; return *this; };

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
	CommandHandlerInfo &addCommandHandler(const char *cmdName, const char *helpStr, std::function<void(SerialCommandParserBase *parser)> handler);

	/**
	 * @brief Add a help command. The default is "help" or "?" but you can override this.
	 *
	 * Typically you call this after adding all of your commands using addCommandHandler().
	 */
	void addHelpCommand(const char *helpCommand = "help|?");

	/**
	 * @brief Get the command handler info structure for a command
	 * 
	 * Returns NULL if the command is not known
	 */
	CommandHandlerInfo *getCommandHandlerInfo(const char *cmd) const;

	const String &getPrompt() const { return prompt; };
	const String &getWelcome() const { return welcome; };

	std::vector<CommandHandlerInfo*> &getCommandHandlers() { return commandHandlers; };

protected:
	std::vector<CommandHandlerInfo*> commandHandlers;
	String prompt;
	String welcome;
};

/**
 * @brief Base class for serial command parsers.
 *
 * You may want to use SerialCommandParser (templated version) instead so the buffers can be allocated
 * statically on the heap at compile time and fewer parameters are involved.
 */
class SerialCommandParserBase : public Print {
public:
	enum class StreamType {
		NONE,
		USARTSerial,
		USBSerial,
		Stream
	};
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
	virtual void clear();

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

	/**
	 * @brief Virtual override class Print
	 */
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
	SerialCommandParserBase &withSerial(USARTSerial *serial) { this->streamType = StreamType::USARTSerial; this->stream = serial; return *this; };

	/**
	 * @brief Sets the USB serial port to read/write to
	 *
	 * This overload is used for USB serial ports: Serial, USBSerial1.
	 */
	SerialCommandParserBase &withSerial(USBSerial *serial) { this->streamType = StreamType::USBSerial; this->stream = serial; return *this; };

	/**
	 * @brief Sets a Stream to read/write to. This is used for TCPClient.
	 *
	 */
	SerialCommandParserBase &withStream(Stream *stream) { this->streamType = StreamType::Stream; this->stream = stream; return *this; };

#endif /* UNITTEST */

	SerialCommandParserBase &withConfig(SerialCommandConfig *config) { this->config = config; return *this; };

	/**
	 * @brief Prints the help message.
	 *
	 * This is normally handled automatically by addHelpCommand(), but is separate so the help/usage can be displayed
	 * when you enter an invalid command as well.
	 */
	void printHelp();

	/**
	 * @brief Print a string with lines terminated with \n expanded to \r\n
	 */
	size_t printWithNewLine(const char *str, bool endWithNewLine);

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
	 * @brief Gets the first character of the argument
	 *
	 * @param index The argument to get (0 = first, 1 = second, ...)
	 *
	 * @param defaultValue if the argument index does not exist, return this value
	 */
	char getArgChar(size_t index, char defaultValue) const;

	/**
	 * @brief Get the parsing state for command line options
	 * 
	 * Will only be non-null if you've configured command line option parsing for commands.
	 */
	CommandParsingState *getParsingState() { return parsingState; };

protected:
	char *buffer;
	size_t bufferSize;
	char **argsBuffer;
	size_t argsBufferSize;
	size_t argsCount = 0;
	size_t bufferOffset = 0;
#ifndef UNITTEST
	StreamType streamType = StreamType::NONE;
	Stream *stream = 0;
	bool usbWasConnected = false;
#endif /* UNITTEST */
	SerialCommandConfig *config = 0;
	CommandParsingState *parsingState = 0;
};


template<size_t BUFFER_SIZE, size_t MAX_ARGS>
class SerialCommandParser : public SerialCommandParserBase, public SerialCommandConfig {
public:
	SerialCommandParser() : SerialCommandParserBase(staticBuffer, BUFFER_SIZE, staticArgsBuffer, MAX_ARGS) {
		withConfig(this);
	};
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

	/**
	 * @brief Clear the data in the processor
	 *
	 * You normal don't need to call this as it's cleared after processing a command. This is
	 * used during unit testing.
	 */
	virtual void clear();

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

	virtual void handlePromptWithCallback(std::function<void()> handlePromptCallback);

	virtual void handleCompletion();

	void setBuffer(const char *str, bool atEnd = false);

	virtual void processChar(char c);

	void scrollToView(ScrollView which, bool forceRedraw);

	void redraw(int fromPos = 0);

	void setCursor();

	void printMessage(const char *fmt, ...);

	void printMessageNoPrompt(const char *fmt, ...);

	void vprintMessage(bool prompt, const char *fmt, va_list ap);

	void printMessagePrompt();

	void setTerminalType(TerminalType terminalType) { this->terminalType = terminalType; }

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
	bool promptRendered = false;
	std::function<void(int row, int col)> positionCallback = 0;
	std::function<void()> handlePromptCallback = 0;
};

template<size_t HISTORY_BUFFER_SIZE, size_t BUFFER_SIZE, size_t MAX_ARGS>
class SerialCommandEditor : public SerialCommandEditorBase, public SerialCommandConfig {
public:
	SerialCommandEditor() : SerialCommandEditorBase(staticHistoryBuffer, HISTORY_BUFFER_SIZE, staticBuffer, BUFFER_SIZE, staticArgsBuffer, MAX_ARGS) {
		staticHistoryBuffer[0] = 0;
		withConfig(this);
	};
	virtual ~SerialCommandEditor() {};


protected:
	char staticHistoryBuffer[HISTORY_BUFFER_SIZE];
	char staticBuffer[BUFFER_SIZE];
	char *staticArgsBuffer[MAX_ARGS];
};

#ifndef UNITTEST

class SerialCommandTCPServer; // Forward declaration

class SerialCommandTCPClient {
public:
	SerialCommandTCPClient(SerialCommandTCPServer *server);
	virtual ~SerialCommandTCPClient();

	void setup();
	void loop();

	void setClient(TCPClient client);

	void stop() { client.stop(); };

	bool isConnected() { return client.connected(); };

	bool isAllocated() const { return editor && historyBuffer && buffer && argsBuffer; };

	SerialCommandEditorBase *getEditor() { return editor; };
	SerialCommandParserBase *getParser() { return editor; };

protected:
	SerialCommandTCPServer *server;
	SerialCommandEditorBase *editor = 0;
	char *historyBuffer = 0;
	char *buffer = 0;
	char **argsBuffer = 0;
	TCPClient client;
	bool wasConnected = false;
};

class SerialCommandTCPServer : public SerialCommandConfig {
public:
	SerialCommandTCPServer(size_t historyBufSize, size_t bufferSize, size_t maxArgs, size_t maxSessions, bool preallocate, uint16_t port);
	virtual ~SerialCommandTCPServer();

	void setup();
	void loop();

	bool isNetworkConnected();

	void stop(SerialCommandParserBase *parser);


protected:
	size_t historyBufSize;
	size_t bufferSize;
	size_t maxArgs;
	size_t maxSessions;
	bool preallocate;
	bool networkWasConnected = false;
	SerialCommandTCPClient **clients = 0;
	TCPServer server;
	friend class SerialCommandTCPClient;
};

#endif /* UNITTEST */

#ifndef UNITTEST

/**
 * @brief Create a LogHandler that logs to SerialCommandEditor.
 *
 * Instead of using this class you'll probably use SerialCommandEditorLogHandler instead, which statically allocates
 * the buffer for you.
 *
 * Use this instead of `SerialLogHandler` if you are using USB serial for the command editor. It
 * will properly mix in the log messages into the terminal output, then restore the command prompt
 * and anything you've typed below it. This makes it much easier to enter commands while
 * serial logging statements are being written.
 *
 */
class SerialCommandEditorLogHandlerBuffer : public StreamLogHandler, public Print {
public:
	/**
	 * @brief Constructor. The object is normally instantiated as a global object.
	 *
	 * @param ringBuffer Buffer pointer
	 * @param ringBufferSize Buffer size. This must be large enough to hold all of the data that could be logged between calls to loop()
	 * @param commandEditor The SerialCommandEditor to write to
	 * @param level  (optional, default is LOG_LEVEL_INFO)
	 * @param filters (optional, default is none)
	 */
	SerialCommandEditorLogHandlerBuffer(uint8_t *ringBuffer, size_t ringBufferSize, SerialCommandEditorBase *commandEditor, LogLevel level = LOG_LEVEL_INFO, LogCategoryFilters filters = {});
	virtual ~SerialCommandEditorLogHandlerBuffer();

	/**
	 * @brief Must be called from setup
	 *
	 * On Gen 3 devices, it's not safe to set up the log handler at global object construction time and you will likely
	 * fault.
	 */
	void setup();

    /**
     * @brief Must be called from loop
     *
     * This method must be called from loop(), ideally on every call to loop. The SerialCommandEditor
     * is not thread safe, so we can only write to it from loop().
     */
    void loop();

    /**
     * @brief Override Print. Called by StreamLogHandler to log data.
     */
    size_t write(uint8_t c);

    void close(SerialCommandParserBase *parser);

    /**
     * @brief Maximum line length
     *
     * If the LogHandler gets a line longer than this, it's broken into multiple lines.
     */
    static const size_t MAX_LINE_LEN = 128;

protected:
    RingBuffer<uint8_t> ringBuffer;
    SerialCommandEditorBase *commandEditor;
    char lineBuffer[MAX_LINE_LEN];
    size_t lineBufferOffset = 0;
};


/**
 * @brief Add a LogHandler logger than intermixes the output with command editing
 *
 * Use this instead of `SerialLogHandler` if you are using USB serial for the command editor. It
 * will properly mix in the log messages into the terminal output, then restore the command prompt
 * and anything you've typed below it. This makes it much easier to enter commands while
 * serial logging statements are being written.
 *
 * Important: Do not enable SERIAL_COMMAND_DEBUG_LEVEL 2 when using this, as it will begin
 * logging recursively and bad things will happen!
 */
template<size_t BUFFER_SIZE>
class SerialCommandEditorLogHandler : public SerialCommandEditorLogHandlerBuffer {
public:
	/**
	 * @brief Constructor. The object is normally instantiated as a global object.
	 *
	 * @param commandEditor The SerialCommandEditor to write to
	 * @param level  (optional, default is LOG_LEVEL_INFO)
	 * @param filters (optional, default is none)
	 */
	explicit SerialCommandEditorLogHandler(SerialCommandEditorBase *commandEditor, LogLevel level = LOG_LEVEL_INFO, LogCategoryFilters filters = {}) :
		SerialCommandEditorLogHandlerBuffer(staticBuffer, sizeof(staticBuffer), commandEditor, level, filters) {};

protected:
	uint8_t staticBuffer[BUFFER_SIZE];
};
#endif /* UNITTEST */


#endif /* __SERIALCOMMANDPARSERRK_H */

# Serial Command Parser

This library is intended to provide a way of accepting commands and options from
a USB or UART serial interface on a Particle device.

Right now, it's a work-in-progress that was needed for another project. 

It can accept simple commands from USB or UART serial, parse arguments, and dispatch. This would be useful if you're driving commands from an automated test.

It can also run in interactive terminal mode with ANSI terminal programs (including screen on Mac and Linux, CoolTerm and PuTTY on Windows). This mode supports:

- Line editing.
- Arrow keys, Home, End, Forward Delete, etc.
- History buffer so you can pull up previously typed commands easily.
- GNU readline/emacs control keys like:
  - Ctrl-A Move cursor to start of line (Home)
  - Ctrl-B Move cursor back (left arrow)
  - Ctrl-E Move cursor to end of line (End)
  - Ctrl-F Move cursor forward (right arrow)
  - Ctrl-H Delete previous character (backspace)
  - Ctrl-I Command completion (tab)
  - Ctrl-K Clear content after cursor
  - Ctrl-L Clear screen
  - Ctrl-N Next Command (down arrow)
  - Ctrl-P Previous Command (up arrow)
- Smart mixing of LogHandler and editing output (optional)

Some future useful features might include:

- Command option processing

## Version History

### 0.0.9 (2025-11-14)

- Fix conflicting definition for `log`.
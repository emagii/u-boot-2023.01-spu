/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2012
 * Pali Rohár <pali@kernel.org>
 */

/*
 * ANSI terminal
 */

#define ANSI_CURSOR_UP			"\e[%dA"
#define ANSI_CURSOR_DOWN		"\e[%dB"
#define ANSI_CURSOR_FORWARD		"\e[%dC"
#define ANSI_CURSOR_BACK		"\e[%dD"
#define ANSI_CURSOR_NEXTLINE		"\e[%dE"
#define ANSI_CURSOR_PREVIOUSLINE	"\e[%dF"
#define ANSI_CURSOR_COLUMN		"\e[%dG"
#define ANSI_CURSOR_POSITION		"\e[%d;%dH"
#define ANSI_CURSOR_SHOW		"\e[?25h"
#define ANSI_CURSOR_HIDE		"\e[?25l"
#define ANSI_CLEAR_CONSOLE		"\e[2J"
#define ANSI_CLEAR_LINE_TO_END		"\e[0K"
#define ANSI_CLEAR_LINE			"\e[2K"
#define ANSI_COLOR_RESET		"\e[0m"
#define ANSI_COLOR_REVERSE		"\e[7m"

#define ANSI_COLOR_BLACK		"\e[30m"
#define ANSI_COLOR_RED			"\e[31m"
#define ANSI_COLOR_GREEN		"\e[32m"
#define ANSI_COLOR_YELLOW		"\e[33m"
#define ANSI_COLOR_BLUE			"\e[34m"
#define ANSI_COLOR_MAGENTA		"\e[35m"
#define ANSI_COLOR_CYAN			"\e[36m"
#define ANSI_COLOR_WHITE		"\e[37m"
#define ANSI_COLOR_GREY			"\e[90m"
#define ANSI_COLOR_BRIGHT_RED		"\e[91m"
#define ANSI_COLOR_BRIGHT_GREEN		"\e[92m"
#define ANSI_COLOR_BRIGHT_YELLOW	"\e[93m"
#define ANSI_COLOR_BRIGHT_BLUE		"\e[94m"
#define ANSI_COLOR_BRIGHT_MAGENTA	"\e[95m"
#define ANSI_COLOR_BRIGHT_CYAN		"\e[96m"
#define ANSI_COLOR_BRIGHT_WHITE		"\e[97m"

#ifndef	ANSI_STD_COLOR
#define	ANSI_STD_COLOR
#endif

/* These macros allows constant strings to have colour
 * If ANSI_STD_COLOR is defined, the colour will return to the std afterwards
 * If it is not defined, you can do BLACK("") to just change colour
 */

#define	BLACK(x)			ANSI_COLOR_BLACK	  x ANSI_STD_COLOR
#define	RED(x)				ANSI_COLOR_RED		  x ANSI_STD_COLOR
#define	GREEN(x)			ANSI_COLOR_GREEN	  x ANSI_STD_COLOR
#define	YELLOW(x)			ANSI_COLOR_YELLOW	  x ANSI_STD_COLOR
#define	BLUE(x)				ANSI_COLOR_BLUE		  x ANSI_STD_COLOR
#define	MAGENTA(x)			ANSI_COLOR_MAGENTA	  x ANSI_STD_COLOR
#define	CYAN(x)				ANSI_COLOR_CYAN		  x ANSI_STD_COLOR
#define	WHITE(x)			ANSI_COLOR_WHITE	  x ANSI_STD_COLOR
#define	GREY(x)				ANSI_COLOR_GREY		  x ANSI_STD_COLOR
#define	BRIGHT_RED(x)			ANSI_COLOR_BRIGHT_RED	  x ANSI_STD_COLOR
#define	BRIGHT_GREEN(x)			ANSI_COLOR_BRIGHT_GREEN	  x ANSI_STD_COLOR
#define	BRIGHT_YELLOW(x)		ANSI_COLOR_BRIGHT_YELLOW  x ANSI_STD_COLOR
#define	BRIGHT_BLUE(x)			ANSI_COLOR_BRIGHT_BLUE	  x ANSI_STD_COLOR
#define	BRIGHT_MAGENTA(x)		ANSI_COLOR_BRIGHT_MAGENTA x ANSI_STD_COLOR
#define	BRIGHT_CYAN(x)			ANSI_COLOR_BRIGHT_CYAN	  x ANSI_STD_COLOR
#define	BRIGHT_WHITE(x)			ANSI_COLOR_BRIGHT_WHITE	  x ANSI_STD_COLOR

#define	_BLACK(x)			ANSI_COLOR_BLACK,	  x
#define	_RED(x)				ANSI_COLOR_RED	,	  x
#define	_GREEN(x)			ANSI_COLOR_GREEN,	  x
#define	_YELLOW(x)			ANSI_COLOR_YELLOW,	  x
#define	_BLUE(x)			ANSI_COLOR_BLUE	,	  x
#define	_MAGENTA(x)			ANSI_COLOR_MAGENTA,	  x
#define	_CYAN(x)			ANSI_COLOR_CYAN	,	  x
#define	_WHITE(x)			ANSI_COLOR_WHITE,	  x
#define	_GREY(x)			ANSI_COLOR_GREY	,	  x
#define	_BRIGHT_RED(x)			ANSI_COLOR_BRIGHT_RED,	  x
#define	_BRIGHT_GREEN(x)		ANSI_COLOR_BRIGHT_GREEN,  x
#define	_BRIGHT_YELLOW(x)		ANSI_COLOR_BRIGHT_YELLOW  x
#define	_BRIGHT_BLUE(x)			ANSI_COLOR_BRIGHT_BLUE,	  x
#define	_BRIGHT_MAGENTA(x)		ANSI_COLOR_BRIGHT_MAGENTA x
#define	_BRIGHT_CYAN(x)			ANSI_COLOR_BRIGHT_CYAN,	  x
#define	_BRIGHT_WHITE(x)		ANSI_COLOR_BRIGHT_WHITE,  x

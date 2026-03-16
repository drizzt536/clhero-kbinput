// compile instructions: make
// 7z a -t7z -mx=9 clhero-kbinput.7z clhero-kbinput.exe

/*
 * exit codes:
 *  - 0: success
 *  - 1: generic error. (look at exit message)
 *  - 2: out of memory
 *
 * relevant links:
 *  - https://github.com/raphaelgoulart/ya_inputdisplay
 *      > inspiration program. controller version.
 *  - https://github.com/raysan5/raylib
 *      > graphics library
 *  - https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
 *      > key codes for non-letter, non-function, non-arrow keys
*/

// TODO: remember the previous size and position of the window through program exits.
//       probably use a config text file for easy editing.
// TODO: consider looking into <xinput.h> or hidapi for controller support?
// TODO: fix bugs:
//       - when you make a note, the most recent not disappears for one frame.
//       - sometimes the notes will jump back and forth by a bit.
// TODO: have a way to change the colors of everything.
//       the outer fret things should be around 112/159 times the inner.
//       the note colors should either be custom, or derived from the fret colors
//       this probably requires refactoring how the colors work.
// TODO: make it so it closes other instances of the program if you open a new one.
// TODO: finish the `kpct` (key press count) stuff.
// TODO: actually handle bind changes after you click on a fret.
// TODO: add default keybinds to the help text?
//       at least mention that backspace removes notes and resets input counts.

#ifndef CFGFILE
	#define CFGFILE "./clhero-kbinput.cfg"
#endif

#ifndef FPS_DEFAULT
	#define FPS_DEFAULT 1000
#endif

#ifndef SPEED_DEFAULT
	#define SPEED_DEFAULT 400
#endif

#ifndef VKEY_DEFAULT_0
	#define VKEY_DEFAULT_0 'A'
#endif

#ifndef VKEY_DEFAULT_1
	#define VKEY_DEFAULT_1 'S'
#endif

#ifndef VKEY_DEFAULT_2
	#define VKEY_DEFAULT_2 'D'
#endif

#ifndef VKEY_DEFAULT_3
	#define VKEY_DEFAULT_3 'F'
#endif

#ifndef VKEY_DEFAULT_4
	#define VKEY_DEFAULT_4 'G'
#endif

#ifndef VKEY_DEFAULT_S1
	#define VKEY_DEFAULT_S1 VK_UP
#endif

#ifndef VKEY_DEFAULT_S2
	#define VKEY_DEFAULT_S2 VK_DOWN
#endif


#ifndef _WIN64
	#error "this program only works on 64-bit Windows."
#endif

#ifndef __GNUC__
	#error "this program will only compile correctly with GCC"
#endif

// fixed-width integers:
typedef char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned u32;
typedef long long i64;
typedef unsigned long long u64;
typedef __int128 i128;
typedef unsigned __int128 u128;

// NOTE: `#include <windows.h>` breaks everything due to name conflicts with raylib.h
#define MB_OK 0
#define FALSE ((BOOL) 0)
#include <winuser.rh>
typedef i32 BOOL;
typedef void *HANDLE, *HINSTANCE, *HWND;
typedef struct {
	u32 dwSize;
	BOOL bVisible;
} CONSOLE_CURSOR_INFO;
// NOTE: CONST is just const, and CHAR is just char.
// typedef const char *LPCSTR;
// typedef const char *LPCTSTR;
// typedef BOOL WINBOOL;
// #define WINAPI __stdcall
void AllocConsole(void);
void __stdcall Sleep(u32 ms);
BOOL ShowWindow(HWND window, i32 nCmdShow);
BOOL IsIconic(HWND window);
BOOL BringWindowToTop(HWND window);
i16 GetAsyncKeyState(i32 key);
i32 MessageBoxA(HWND parent_window, const char *text, const char *caption, u32 type);
HANDLE __stdcall GetStdHandle(u32 nStdHandle);
HWND __stdcall GetConsoleWindow(void);
BOOL __stdcall SetConsoleTitleA(const char *title);
BOOL __stdcall GetConsoleMode(HANDLE handle, u32 *mode);
BOOL __stdcall SetConsoleMode(HANDLE handle, u32 mode);
BOOL __stdcall GetConsoleCursorInfo(HANDLE consoleOutput, CONSOLE_CURSOR_INFO *consoleCursorInfo);
BOOL __stdcall SetConsoleCursorInfo(HANDLE consoleOutput, const CONSOLE_CURSOR_INFO *consoleCursorInfo);

// ucrt functions
#define NULL ((void *) 0)
// #define SEEK_SET 0
typedef void FILE; // opaque type. It should work if I pretend it is void.
i32 printf(const char *fmt, ...);
i32 fprintf(FILE *stream, const char *fmt, ...);
i32 vsnprintf(char *str, u64 n, const char *format, __builtin_va_list args);
i32 sprintf(char *str, const char *format, ...);
i32 sscanf(const char *restrict buffer, const char *restrict format, ...);
i32 puts(const char *str);
i32 putchar(i32 ch);
i32 _getch_nolock(void);
i32 strcmp(const char *str1, const char *str2);
i32 strncmp(const char *lhs, const char *rhs, u64 count);
FILE *__acrt_iob_func(u32 index);
u32 strtoul(const char *str, char **endptr, i32 base);
void __attribute__((noreturn)) exit(i32 exitcode);
i32 _crt_atexit(void (*callback)(void));
FILE *fopen(const char *filename, const char * mode);
i32 feof(FILE *stream);
char *fgets(char *restrict str, i32 count, FILE *restrict stream);
// i32 _fseeki64_nolock(FILE *stream, i64 offset, i32 origin);
// i64 _ftelli64_nolock(FILE *stream);
i32 _fclose_nolock(FILE *stream);
FILE *freopen(const char *filename, const char *mode, FILE *stream);
void *malloc(u64 size);
void free(void *ptr);
// #define fseek _fseeki64_nolock
// #define ftell _ftelli64_nolock
#define fclose _fclose_nolock
#define _getch _getch_nolock
#define atexit _crt_atexit

#include "raylib.h"

#define _AF_ATTR(x) __attribute__((cleanup(cleanup_##x)))
#define AUTO_FREE(x) _AF_ATTR(x) x
#define AF_char      _AF_ATTR(array) char
#define AF_LaneArray _AF_ATTR(LaneArray) Lane
#define AF_FILE      AUTO_FREE(FILE) // should be a `FILE *` variable and not just `FILE`
#define FORCE_INLINE inline __attribute__((always_inline, gnu_inline))

#define FONT_SIZE 20

#define EXIT_OOM 2

// STR_IN only works up to four options in the set.
#define streq(s1, s2) (strcmp(s1, s2) == 0)
#define strneq(s1, s2, n) (strncmp(s1, s2, n) == 0)
#define _COUNT_ARGS_IMPL(_1, _2, _3, _4, count, ...) count
#define COUNT_ARGS(...) _COUNT_ARGS_IMPL(__VA_ARGS__ __VA_OPT__(,) 4, 3, 2, 1, 0)
#define _STR_IN_PICK(str, count, ...) STR_IN_ ## count(str, __VA_ARGS__)
#define STR_IN_PICK(str, count, ...) _STR_IN_PICK(str, count, __VA_ARGS__)
#define STR_IN_0(str)					false
#define STR_IN_1(str, m1)				streq(str, m1)
#define STR_IN_2(str, m1, m2)			(streq(str, m1) || STR_IN_1(str, m2))
#define STR_IN_3(str, m1, m2, m3)		(streq(str, m1) || STR_IN_2(str, m2, m3))
#define STR_IN_4(str, m1, m2, m3, m4)	(streq(str, m1) || STR_IN_3(str, m2, m3, m4))
#define STR_IN(str, ...) STR_IN_PICK(str, COUNT_ARGS(__VA_ARGS__), __VA_ARGS__)

#define _CHR_IN_PICK(chr, count, ...) CHR_IN_ ## count(chr, __VA_ARGS__)
#define CHR_IN_PICK(chr, count, ...) _CHR_IN_PICK(chr, count, __VA_ARGS__)
#define CHR_IN_0(chr)					false
#define CHR_IN_1(chr, m1)				(chr == m1)
#define CHR_IN_2(chr, m1, m2)			((chr == m1) || CHR_IN_1(chr, m2))
#define CHR_IN_3(chr, m1, m2, m3)		((chr == m1) || CHR_IN_2(chr, m2, m3))
#define CHR_IN_4(chr, m1, m2, m3, m4)	((chr == m1) || CHR_IN_3(chr, m2, m3, m4))
#define CHR_IN(chr, ...) CHR_IN_PICK(chr, COUNT_ARGS(__VA_ARGS__), __VA_ARGS__)


#define keypressed(vkey) (GetAsyncKeyState(vkey) & 0x8000)

#define BOOL_STR(b) ((b) != 0 ? "true" : "false")

// stringify without expanding
#define _STRINGIFY(x) #x
// stringify with expanding
#define STRINGIFY(x) _STRINGIFY(x)

// background, down, up, note
#define COLOR_STRUM_BG ((Color) {  32,   0,  64, 255 })
#define COLOR_STRUM_DN ((Color) {  80,   0, 159, 255 })
#define COLOR_STRUM_UP ((Color) {  56,   0, 112, 255 })

#define COLOR_LANE0_BG ((Color) {   0,  64,   0, 255 })
#define COLOR_LANE0_DN ((Color) {   0, 159,   0, 255 })
#define COLOR_LANE0_UP ((Color) {   0, 112,   0, 255 })
#define COLOR_LANE0_NT ((Color) {  51, 255,   0, 255 })

#define COLOR_LANE1_BG ((Color) {  64,   0,   0, 255 })
#define COLOR_LANE1_DN ((Color) { 159,   0,   0, 255 })
#define COLOR_LANE1_UP ((Color) { 112,   0,   0, 255 })
#define COLOR_LANE1_NT ((Color) { 255,   0,   0, 255 })

#define COLOR_LANE2_BG ((Color) {  64,  64,   0, 255 })
#define COLOR_LANE2_DN ((Color) { 159, 159,   0, 255 })
#define COLOR_LANE2_UP ((Color) { 112, 112,   0, 255 })
#define COLOR_LANE2_NT ((Color) { 255, 255,   0, 255 })

#define COLOR_LANE3_BG ((Color) {   0,   0, 104, 255 })
#define COLOR_LANE3_DN ((Color) {   0,   0, 216, 255 })
#define COLOR_LANE3_UP ((Color) {   0,   0, 152, 255 })
#define COLOR_LANE3_NT ((Color) {   0, 168, 255, 255 })

#define COLOR_LANE4_BG ((Color) {  64,  32,   0, 255 })
#define COLOR_LANE4_DN ((Color) { 159,  80,   0, 255 })
#define COLOR_LANE4_UP ((Color) { 112,  56,   0, 255 })
#define COLOR_LANE4_NT ((Color) { 255, 160,   0, 255 })

#define DEFAULT_SIZE_X 450
#define DEFAULT_SIZE_Y 800
#define DEFAULT_SIZE_STR STRINGIFY(DEFAULT_SIZE_X) "," STRINGIFY(DEFAULT_SIZE_Y)

#define _DrawRectangleRec(r, c) { DrawRectangleRec(r, c); puts("drawing rectangle"); }

// variables that are used in more than one function.
static const char *const cons_title = "(terminal) Clone Hero Keyboard Input Displayer";
static FILE *stdin, *stdout, *stderr, *cfgfile = NULL;
static bool debug = false;

typedef struct {
	i32 vkey;
	bool down;
} KeyData;

typedef struct {
	u8 n; // number of rectangles
	u8 mr; // the index of the most recent rectangle.
	Rectangle *recs;
	Color color;
	i32 vkey;
	u32 kpct; // key press count
	bool keydown;
} Lane;

static Rectangle // inner and outer fret boxes
	oFret0, iFret0,
	oFret1, iFret1,
	oFret2, iFret2,
	oFret3, iFret3,
	oFret4, iFret4,
	iStrum1, iStrum2,
	oStrum1, oStrum2,
	oStrum;

static i32
	prev_scrwidth = 0,
	cur_scrwidth = 0,
	cur_scrheight = 0;

static float scale = 1,
	note_y  = 0, // note spawn height
	x0      = 0, x1, x2, x3, x4,
	lane_wd = 0; // lane width

static FORCE_INLINE void cleanup_array(const void *p) {
	free(*(void **) p);
}

static FORCE_INLINE void cleanup_FILE(FILE *const *fp) {
	fclose(*fp);
}

static FORCE_INLINE void cleanup_LaneArray(Lane *const *p) {
	Lane *x = *(Lane **) p;

	while (x->recs != NULL) {
		free(x->recs);
		x++;
	}
}

#define ERR_MSG_LEN_CAP 256

void fatal(i32 exitcode, const char *fmt, ...) {
	if (exitcode == EXIT_OOM)
		goto fallback_oom;

	char *const str = malloc(ERR_MSG_LEN_CAP);
	if (str == NULL)
		goto fallback_oom;

	__builtin_va_list args;
	__builtin_va_start(args, fmt);
	vsnprintf(str, ERR_MSG_LEN_CAP, fmt, args);
	__builtin_va_end(args);

	if (debug)
		printf("\e[31mERROR: %s\e[0m\n", str);
	else
		MessageBoxA(NULL /* window */, str, NULL /* title */, MB_OK);

	exit(exitcode);
fallback_oom:
	// ignore all the arguments.
	if (debug)
		puts("out of memory.");
	else
		MessageBoxA(NULL, "out of memory.", NULL, MB_OK);

	exit(EXIT_OOM);
}

void warning(const char *fmt, ...) {
	char *const str = malloc(ERR_MSG_LEN_CAP);
	if (str == NULL)
		fatal(EXIT_OOM, NULL);

	__builtin_va_list args;
	__builtin_va_start(args, fmt);
	vsnprintf(str, ERR_MSG_LEN_CAP, fmt, args);
	__builtin_va_end(args);

	if (debug)
		printf("\e[38;5;172mWARNING: %s\e[0m\n", str);
	else
		MessageBoxA(NULL /* window */, str, "Warning", MB_OK);
}

void enable_debug(void) {
	if (debug) return;

	AllocConsole();
	SetConsoleTitleA(cons_title);

	// enable ANSI escape sequences.
	HANDLE hOut = GetStdHandle(-11); // STD_OUTPUT_HANDLE

	u32 mode = 0;
	GetConsoleMode(hOut, &mode);
	SetConsoleMode(hOut, mode | 4); // ENABLE_VIRTUAL_TERMINAL_PROCESSING

	// hide the cursor
	CONSOLE_CURSOR_INFO cursorInfo;

	GetConsoleCursorInfo(hOut, &cursorInfo);
	cursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo(hOut, &cursorInfo);

	// remap standard files to the new console
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	freopen("CONIN$", "r", stdin);

	puts("ClHero-KbInput v0.2");

	debug = true;
}

void exit_handler(void) {
	if (cfgfile != NULL)
		fclose(cfgfile);

	if (!debug) return;

	const HWND console_window = GetConsoleWindow();

	// unminimize the console if it is minimized.
	// the condition is not required, but it makes for better UX if it wasn't minimized.
	if (IsIconic(console_window))
		ShowWindow(console_window, 1 /* SW_NORMAL */);

	BringWindowToTop(console_window);

	puts("press any key to confirm exit");

	_getch();
}

static FORCE_INLINE void rescale_window(void) {
	prev_scrwidth = cur_scrwidth;
	scale = (float) cur_scrwidth / 300;

	// the minimum height is a function of the width.
	SetWindowMinSize(300, 180*scale);


	const float
		oStrumHt = 25*scale, // outer strum box height
		oStrumWd = (float) cur_scrwidth / 2, // outer strum box width

		bd = 5 * scale, // border width
		y0 = 50*scale,
		len_i = 50*scale; // inner box length

	lane_wd = len_i + bd*2;
	x1 = x0 + lane_wd;
	x2 = x1 + lane_wd;
	x3 = x2 + lane_wd;
	x4 = x3 + lane_wd;

	note_y = y0 + lane_wd;

	// oStrum is used for rendering. oStrum1 and oStrum2 are used for mouse bounds checks.
	oStrum = (Rectangle) {
		.x = 0,
		.y = oStrumHt,
		.width = cur_scrwidth,
		.height = oStrumHt
	};
	oStrum1 = (Rectangle) {
		.x = 0,
		.y = oStrumHt,
		.width = oStrumWd,
		.height = oStrumHt
	};
	iStrum1 = (Rectangle) {
		.x = bd,
		.y = oStrumHt + bd,
		.width = oStrumWd - 2*bd,
		.height = oStrumHt - 2*bd
	};

	oStrum2 = (Rectangle) {
		.x = oStrumWd,
		.y = oStrumHt,
		.width = oStrumWd,
		.height = oStrumHt
	};
	iStrum2 = (Rectangle) {
		.x = oStrumWd + bd,
		.y = oStrumHt + bd,
		.width = oStrumWd - 2*bd,
		.height = oStrumHt - 2*bd
	};

	oFret0 = (Rectangle) {
		.x = x0,
		.y = y0,
		.width = lane_wd,
		.height = lane_wd
	};
	iFret0 = (Rectangle) {
		.x = x0 + bd,
		.y = y0 + bd,
		.width = len_i,
		.height = len_i
	};

	oFret1 = (Rectangle) {
		.x = x1,
		.y = y0,
		.width = lane_wd,
		.height = lane_wd
	};
	iFret1 = (Rectangle) {
		.x = x1 + bd,
		.y = y0 + bd,
		.width = len_i,
		.height = len_i
	};

	oFret2 = (Rectangle) {
		.x = x2,
		.y = y0,
		.width = lane_wd,
		.height = lane_wd
	};
	iFret2 = (Rectangle) {
		.x = x2 + bd,
		.y = y0 + bd,
		.width = len_i,
		.height = len_i
	};

	oFret3 = (Rectangle) {
		.x = x3,
		.y = y0,
		.width = lane_wd,
		.height = lane_wd
	};
	iFret3 = (Rectangle) {
		.x = x3 + bd,
		.y = y0 + bd,
		.width = len_i,
		.height = len_i
	};

	oFret4 = (Rectangle) {
		.x = x4,
		.y = y0,
		.width = lane_wd,
		.height = lane_wd
	};
	iFret4 = (Rectangle) {
		.x = x4 + bd,
		.y = y0 + bd,
		.width = len_i,
		.height = len_i
	};
}

void init_args(void); // exported from assembly. init-crt.o

extern i32 __stdcall WinMainCRTStartup(void) {
	i32 argc;
	char **argv;

	asm volatile (
		"call init_args\n\t"
		"movl %%edi, %0\n\t"
		"movq %%rsi, %1"
		: "=r"(argc), "=r"(argv)
		:: "rax", "rcx", "rdx", "r8", "r9", "rdi", "rsi", "cc", "memory"
	);

	stdin  = __acrt_iob_func(0);
	stdout = __acrt_iob_func(1);
	stderr = __acrt_iob_func(2);
	/////////////////////////////////////////////

	atexit(&exit_handler);

	double speed = SPEED_DEFAULT; // pixels per second
	i32 fps_tgt = FPS_DEFAULT,
		flags = FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN;

	struct { i32 x, y; } window_pos = {};
	struct { u32 x, y; } window_size = {DEFAULT_SIZE_X, DEFAULT_SIZE_Y};

	KeyData strum1 = {VKEY_DEFAULT_S1, false}, strum2 = {VKEY_DEFAULT_S2, false};
	// the rectangle fields are populated to real pointers later.
	AF_LaneArray *const lanes = (Lane [6]) {
		{ .recs = NULL, .color = COLOR_LANE0_NT, .vkey = VKEY_DEFAULT_0 },
		{ .recs = NULL, .color = COLOR_LANE1_NT, .vkey = VKEY_DEFAULT_1 },
		{ .recs = NULL, .color = COLOR_LANE2_NT, .vkey = VKEY_DEFAULT_2 },
		{ .recs = NULL, .color = COLOR_LANE3_NT, .vkey = VKEY_DEFAULT_3 },
		{ .recs = NULL, .color = COLOR_LANE4_NT, .vkey = VKEY_DEFAULT_4 },
		{ .recs = NULL }
	};

	bool transparent  = false;
	bool save_cfgfile = true;

	cfgfile = fopen(CFGFILE, "r");
	if (cfgfile == NULL) {
		// there is no config file, or it is not readable.
		// this isn't necessarily an error, so don't exit.
		goto parse_cli_args;
	}

	// parse the config file.
	{
		#define CFG_LINE_MAX 64

		// NOTE: len should be `strlen(name ": ")`
		#define CFG_PROCESS_FLAG_BOOL(name, len, flag) ({ \
			if (strneq(line + len, "true", 4) && CHR_IN(line[len + 4], '\n', '\0')) \
				flags |= flag; \
			else { \
				if (!strneq(line + len, "false", 5) || !CHR_IN(line[len + 5], '\n', '\0')) \
					warning("Invalid config file `%s` attribute at line %u. " \
						"reverting to default (\"%s\").", name, i, "false"); \
				flags &= ~flag; \
			} \
		})

		// putting it on the heap is not necessary for this small of a buffer.
		char line[CFG_LINE_MAX];

		for (u8 i = 1; fgets(line, CFG_LINE_MAX, cfgfile) != NULL; i++) {
			// i is the line index. assume there are less than 255 lines.
			// breaks on no more lines or on an error.

			if (*line == '\n')
				// empty line
				continue;

			// make sure the line is a valid length.
			// combining longer lines together is too difficult for the tiny benefit.
			for (u8 j = 0; j < CFG_LINE_MAX; j++) {
				if (line[j] == '\n')
					break;

				// NOTE: the `!feof(cfcfile)` part is so it doesn't complain
				//       if there is no newline at the end of the file.
				if (line[j] == '\0' && !feof(cfgfile))
					fatal(1, "config file line %u longer than " STRINGIFY(CFG_LINE_MAX) " characters.", i);
			}

			if (*line == '#') {
				if (debug)
					printf("%s", line);

				continue;
			}

			if (strneq(line, "debug: ", 7)) {
				// 7 == strlen("debug: ")

				if (strneq(line + 7, "true", 4) && CHR_IN(line[7 + 4], '\n', '\0'))
					enable_debug();
				// NOTE: `debug: true\ndebug: false` doesn't re-disable it. debug cannot be disabled once enabled.
				//       because I don't feel like allowing it, not because I can't allow it.
			}
			else if (strneq(line, "vsync: ", 7))
				CFG_PROCESS_FLAG_BOOL("vsync", 7, FLAG_VSYNC_HINT);
			else if (strneq(line, "top: ", 5))
				CFG_PROCESS_FLAG_BOOL("top", 5, FLAG_WINDOW_TOPMOST);
			else if (strneq(line, "borderless: ", 12))
				CFG_PROCESS_FLAG_BOOL("borderless", 12, FLAG_WINDOW_UNDECORATED);
			else if (strneq(line, "transparent: ", 13))
				CFG_PROCESS_FLAG_BOOL("transparent", 13, FLAG_WINDOW_TRANSPARENT);
			else if (strneq(line, "fullscreen: ", 12)) {
				if (strneq(line + 12, "true", 4) && CHR_IN(line[12 + 4], '\n', '\0')) {
					flags |= FLAG_WINDOW_MAXIMIZED;
					window_size.x = 0;
					window_size.y = 0;
				}
				else {
					if (!strneq(line + 12, "false", 5) || !CHR_IN(line[12 + 5], '\n', '\0'))
						warning("Invalid config file `%s` attribute at line %u. "
							"reverting to default (\"%s\").", "fullscreen", i, "false");

					flags &= ~FLAG_WINDOW_MAXIMIZED;
					window_size.x = DEFAULT_SIZE_X;
					window_size.y = DEFAULT_SIZE_Y;
				}
			}
			else if (strneq(line, "pos:", 4)) {
				if (sscanf(line + 4, "%d,%d", &window_pos.x, &window_pos.y) != 2) {
					warning("Invalid config file `%s` attribute at line %u. reverting to default (\"%s\").", "pos", i, "0,0");
					window_pos.x = window_pos.y = 0;
				}
			}
			else if (strneq(line, "size:", 5)) {
				if (sscanf(line + 5, "%u,%u", &window_size.x, &window_size.y) != 2) {
					warning("Invalid config file `%s` attribute at line %u. reverting to default (\"%s\").", "size", i, DEFAULT_SIZE_STR);
					window_size.x = DEFAULT_SIZE_X;
					window_size.y = DEFAULT_SIZE_Y;
				}
			}
			else if (strneq(line, "fps: ", 5)) {
				if (strneq(line + 5, "uncapped", 8) && CHR_IN(line[5 + 8], '\n', '\0'))
					fps_tgt = 0;
				else if (sscanf(line + 5, "%u", &fps_tgt) != 1) {
					warning("Invalid config file `%s` attribute at line %u. reverting to default (\"%s\").", "fps", i, STRINGIFY(FPS_DEFAULT));
					fps_tgt = FPS_DEFAULT;
				}
			}
			else if (strneq(line, "speed: ", 7)) {
				unsigned _speed;
				if (sscanf(line + 7, "%u", &_speed) != 1) {
					warning("Invalid config file `%s` attribute at line %u. reverting to default (\"%s\").", "speed", i, STRINGIFY(SPEED_DEFAULT));
					_speed = SPEED_DEFAULT;
				}

				speed = (double) _speed;
			}
			else if (strneq(line, "key: ", 5)) {
				u8 idx;
				i32 vkey;
				i32 rc = sscanf(line + 5, "%c,0x%x", &idx, &vkey);

				if ((idx < '0' || '4' < idx) && idx != 'U' && idx != 'D') {
					warning("Invalid config file `key` (%c) attribute at line %u. ignored", idx, i);
					goto endloop;
				}

				if (rc != 2) {
					char tmp;

					if ((tmp = line[7]) == '\0') {
						warning("Invalid config file `key` (%c) attribute at line %u. ignored", idx, i);
						goto endloop;
					}

					if (tmp == 'F' || tmp == 'f') {
						// parse f1-f24
						u32 tmp2;

						if (sscanf(line + 8, "%u", &tmp2) != 0) {
							switch (tmp2) {
							case  1: vkey = VK_F1;  break;
							case  2: vkey = VK_F2;  break;
							case  3: vkey = VK_F3;  break;
							case  4: vkey = VK_F4;  break;
							case  5: vkey = VK_F5;  break;
							case  6: vkey = VK_F6;  break;
							case  7: vkey = VK_F7;  break;
							case  8: vkey = VK_F8;  break;
							case  9: vkey = VK_F9;  break;
							case 10: vkey = VK_F10; break;
							case 11: vkey = VK_F11; break;
							case 12: vkey = VK_F12; break;
							case 13: vkey = VK_F13; break;
							case 14: vkey = VK_F14; break;
							case 15: vkey = VK_F15; break;
							case 16: vkey = VK_F16; break;
							case 17: vkey = VK_F17; break;
							case 18: vkey = VK_F18; break;
							case 19: vkey = VK_F19; break;
							case 20: vkey = VK_F20; break;
							case 21: vkey = VK_F21; break;
							case 22: vkey = VK_F22; break;
							case 23: vkey = VK_F23; break;
							case 24: vkey = VK_F24; break;
							default: vkey = tmp;    break; // just use f or F
							}
						}
					} // f or F
					else if (strneq(line + 7, "LEFT", 4) && CHR_IN(line[7 + 4], '\n', '\0'))
						vkey = VK_LEFT;
					else if (strneq(line + 7, "UP", 2) && CHR_IN(line[7 + 2], '\n', '\0'))
						vkey = VK_UP;
					else if (strneq(line + 7, "RIGHT", 5) && CHR_IN(line[7 + 5], '\n', '\0'))
						vkey = VK_RIGHT;
					else if (strneq(line + 7, "DOWN", 4) && CHR_IN(line[7 + 4], '\n', '\0'))
						vkey = VK_DOWN;
					else vkey = (i32) tmp;
				} // rc != 2

				switch (idx) {
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
						lanes[idx - '0'].vkey = vkey;
						break;
					case 'U':
						strum1.vkey = vkey;
						break;
					case 'D':
						strum2.vkey = vkey;
						break;
					default:
						__builtin_unreachable();
				}
			}
			/*else if (strneq(line, "key0: ", 6)) {
				if (sscanf(line + 6, "%u", &lanes[0].vkey) != 1) {
					warning("Invalid config file `%s` attribute at line %u. reverting to default (\"%s\").", "key0", i, STRINGIFY(VKEY_DEFAULT_0));
					lanes[0].vkey = VKEY_DEFAULT_0;
				}
			}
			else if (strneq(line, "key1: ", 6)) {
				if (sscanf(line + 6, "%u", &lanes[1].vkey) != 1) {
					warning("Invalid config file `%s` attribute at line %u. reverting to default (\"%s\").", "key1", i, STRINGIFY(VKEY_DEFAULT_1));
					lanes[1].vkey = VKEY_DEFAULT_1;
				}
			}
			else if (strneq(line, "key2: ", 6)) {
				if (sscanf(line + 6, "%u", &lanes[2].vkey) != 1) {
					warning("Invalid config file `%s` attribute at line %u. reverting to default (\"%s\").", "key2", i, STRINGIFY(VKEY_DEFAULT_2));
					lanes[2].vkey = VKEY_DEFAULT_2;
				}
			}
			else if (strneq(line, "key3: ", 6)) {
				if (sscanf(line + 6, "%u", &lanes[3].vkey) != 1) {
					warning("Invalid config file `%s` attribute at line %u. reverting to default (\"%s\").", "key3", i, STRINGIFY(VKEY_DEFAULT_3));
					lanes[3].vkey = VKEY_DEFAULT_3;
				}
			}
			else if (strneq(line, "key4: ", 6)) {
				if (sscanf(line + 6, "%u", &lanes[4].vkey) != 1) {
					warning("Invalid config file `%s` attribute at line %u. reverting to default (\"%s\").", "key4", i, STRINGIFY(VKEY_DEFAULT_4));
					lanes[4].vkey = VKEY_DEFAULT_4;
				}
			}
			else if (strneq(line, "strum1: ", 8)) {
				if (sscanf(line + 8, "%u", &strum1.vkey) != 1) {
					warning("Invalid config file `%s` attribute at line %u. reverting to default (\"%s\").", "strum1", i, STRINGIFY(VKEY_DEFAULT_S1));
					strum1.vkey = VKEY_DEFAULT_S1;
				}
			}
			else if (strneq(line, "strum2: ", 8)) {
				if (sscanf(line + 8, "%u", &strum2.vkey) != 1) {
					warning("Invalid config file `%s` attribute at line %u. reverting to default (\"%s\").", "strum2", i, STRINGIFY(VKEY_DEFAULT_S2));
					strum2.vkey = VKEY_DEFAULT_S2;
				}
			}*/
			else {
				warning("Unknown config file attribute at line %u.", i);
				continue;
			}
		endloop:
			if (debug)
				printf("%s", line);

			// TODO: add other attributes.
		} // for loop


		if (!feof(cfgfile))
			warning("config file is longer than 255 lines. remaining lines are ignored");
	} // end block for config file parsing.


parse_cli_args:
	putchar('\n');
	// CLI arguments override config file arguments.
	for (i32 i = 1; i < argc; i++) {
		if (streq(argv[i], "--debug"))
			enable_debug();
		else if (streq(argv[i], "--version")) {
			enable_debug();
			exit(0);
		}
		else if (STR_IN(argv[i], "--help", "-h", "-?")) {
			enable_debug();

			puts(
				"\noptions:"
				"\n    --help, -h, -?  print this message and exit."
				"\n    --version       print the version and exit."
				"\n    --debug         allocate a console for debug information."
				"\n    --fps FPS       specify a target fps for raylib. must be a non-negative integer."
				"\n                    pass 0 or \"uncapped\" to remove the FPS cap."
				"\n    --speed PPS     specify a note speed in pixels per second. default is 400"
				"\n                    can be any integer between 0 and 2^24, inclusive."
				"\n    --pos X,Y       specify a starting window position. default is whatever GLFW decides."
				"\n    --size W,H      specify a starting window position. 0,0 for fullscreen."
				"\n                    default is " DEFAULT_SIZE_STR "."
				"\n    --top           display window always on top."
				"\n    --borderless    display without the toolbar buttons at the top."
				"\n    --transparent   display with a transparent window."
				"\n    --fullscreen    display the window in fullscreen mode. implies `--size 0,0`"
				"\n    --vsync         try to enable vsync. only works if fps > refresh rate."
				"\n    --nosave        don't update the config file, and don't generate it if it doesn't exist."
				"\n"
				"\nthe config file will be placed at ./" CFGFILE "."
				"\nkey codes here: https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes"
				"\nf1-f24 (or F1-F24), UP, DOWN, LEFT, RIGHT, and A-Z (uppercase only) can be given directly,"
				"\nbut any other keys must be given as a virtual key code, e.g. for spacebar, put 0x20"
				"\n" // double newline.
			);

			exit(0);
		}
		else if (streq(argv[i], "--fps")) {
			i++;

			if (i == argc)
				fatal(1, "`%s` given without a value.", "--fps");

			if (streq(argv[i], "uncapped")) {
				fps_tgt = 0;
				continue;
			}

			char *end;
			fps_tgt = strtoul(argv[i], &end, 0);

			if (*end != '\0' || fps_tgt < 0)
				fatal(1, "`%s` given with an invalid value `%s`.", "--fps", argv[i]);
		}
		else if (streq(argv[i], "--speed")) {
			i++;

			if (i == argc)
				fatal(1, "`%s` given without a value.", "--speed");

			char *end;
			u32 _speed = strtoul(argv[i], &end, 0);

			if (*end != '\0' || _speed > 0x100000)
				fatal(1, "`%s` given with an invalid value `%s`.", "--speed", argv[i]);

			speed = (double) _speed;
		}
		else if (streq(argv[i], "--pos")) {
			i++;

			if (i == argc)
				fatal(1, "`%s` given without a value.", "--pos");

			if (sscanf(argv[i], "%d,%d", &window_pos.x, &window_pos.y) != 2)
				fatal(1, "`%s` given with an invalid value `%s`.", "--pos", argv[i]);
		}
		else if (streq(argv[i], "--size")) {
			i++;

			if (i == argc)
				fatal(1, "`%s` given without a value.", "--size");

			if (sscanf(argv[i], "%d,%d", &window_size.x, &window_size.y) != 2)
				fatal(1, "`%s` given with an invalid value `%s`.", "--size", argv[i]);
		}
		else if (streq(argv[i], "--transparent")) {
			transparent = true;
			flags |= FLAG_WINDOW_TRANSPARENT;
		}
		else if (streq(argv[i], "--fullscreen")) {
			// FLAG_BORDERLESS_WINDOWED_MODE seems to have the same effect
			flags |= FLAG_WINDOW_MAXIMIZED;
			window_size.x = 0;
			window_size.y = 0;
		}
		else if (streq(argv[i], "--top"))
			flags |= FLAG_WINDOW_TOPMOST;
		else if (streq(argv[i], "--borderless"))
			// NOTE: not FLAG_BORDERLESS_WINDOWED_MODE.
			flags |= FLAG_WINDOW_UNDECORATED;
		else if (streq(argv[i], "--vsync"))
			flags |= FLAG_VSYNC_HINT;
		else if (streq(argv[i], "--nosave"))
			save_cfgfile = false;
		else
			fatal(1, "unknown argument `%s` at position %d.\nuse `--help` for help.", argv[i], i);
	}

	if (debug) printf(
		"using fps target: %d\n"
		"using speed: %lg\n"
		"using window pos: %d,%d\n"
		"using window size: %u,%u\n"
		"----------------------------------------------------\n",
		fps_tgt, speed, window_pos.x, window_pos.y,
		window_size.x, window_size.y
	);
	// end of CLI setup

	u8 mouse_over = 0;

	double move_diff;
	u32 fps;

	bool keydn[5] = {};
	bool strumdown1 = false,
		strumdown2 = false;

	Vector2 mousepos; // only used for click positions.

	// these two are only used in debug mode.
	AF_char *fps_str = malloc(16); // 1 + strlen("FPS: 2147483647")
	AF_char *objs_str = malloc(36); // 1 + strlen("objs: 1290 (15 255 255 255 255 255)")

	if (fps_str == NULL || objs_str == NULL)
		fatal(EXIT_OOM, NULL);

	// NOTE: these are never freed because they are used until exit.
	for (u8 i = 0; i < 5; i++) {
		lanes[i].recs = malloc(256 * sizeof(Rectangle));

		if (lanes[i].recs == NULL)
			fatal(EXIT_OOM, NULL);
	}

	// lane[4].recs == NULL implies lane[0].recs through lane[3].recs are also NULL.
	// because the allocations are the same size, with no possible `free`s in between.
	// the fps_str allocation is smaller and at the start, so that check isn't necessary either
	// also it is the start of the program, so why would it be out of memory?


	SetConfigFlags(flags);
	InitWindow(
		window_size.x,
		window_size.y,
		cons_title + 11 // 11 == strlen("(terminal) ")
	);
	if (window_pos.x != 0 || window_pos.y != 0)
		// while (0, 0) is a valid location, just pretend it isn't.
		// if you want it to be in that corner, just use (1, 1) or (0, 1) or something.
		SetWindowPosition(window_pos.x, window_pos.y);

	SetTargetFPS(fps_tgt);

	// NOTE: 300 is picked so `scale` is never less than 1.
	//       180 is so it always shows the frets and a little bit more past that.
	//       if these numbers change, they also need to change in `rescale_window`.
	SetWindowMinSize(300, 180);

	while (!WindowShouldClose()) {
		fps = GetFPS();
		cur_scrheight = GetRenderHeight();
		mousepos = GetMousePosition();
		move_diff = speed * GetFrameTime(); // NOTE: the frame time isn't used anywhere else.

		if ((cur_scrwidth = GetRenderWidth()) != prev_scrwidth) {
			float prev_scale = scale;
			rescale_window();

			float factor = scale / prev_scale;
			for (u8 i = 0; i < 5; i++) {
				Lane *lane = lanes + i;
				Rectangle *recs = lane->recs;

				for (u8 j = 0; j < lane->n; j++) {
					recs[j].x      *= factor;
					recs[j].y      *= factor;
					recs[j].width  *= factor;
					recs[j].height *= factor;
				}
			}
		}

		strum1.down = strumdown1; strumdown1 = keypressed(strum1.vkey);
		strum2.down = strumdown2; strumdown2 = keypressed(strum2.vkey);
		for (u8 i = 0; i < 5; i++) {
			lanes[i].keydown = keydn[i];

			if ((keydn[i] = keypressed(lanes[i].vkey)) && !lanes[i].keydown)
				lanes[i].kpct++;
		}

		// delete all the notes and set the key counts to zero.
		if (keypressed(VK_BACK))
			for (u8 i = 0; i < 5; i++)
				lanes[i].n = lanes[i].mr = lanes[i].kpct = lanes[i].keydown = 0;


		mouse_over = 7; // default value
		// NOTE: mouse_over only matters if the button was just clicked.

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			if      (CheckCollisionPointRec(mousepos, oFret0))  mouse_over = 0;
			else if (CheckCollisionPointRec(mousepos, oFret1))  mouse_over = 1;
			else if (CheckCollisionPointRec(mousepos, oFret2))  mouse_over = 2;
			else if (CheckCollisionPointRec(mousepos, oFret3))  mouse_over = 3;
			else if (CheckCollisionPointRec(mousepos, oFret4))  mouse_over = 4;
			else if (CheckCollisionPointRec(mousepos, oStrum1)) mouse_over = 5;
			else if (CheckCollisionPointRec(mousepos, oStrum2)) mouse_over = 6;
		}

		BeginDrawing();
		ClearBackground(transparent ? BLANK : BLACK);

		// purple
		DrawRectangleRec(oStrum, COLOR_STRUM_BG);
		DrawRectangleRec(iStrum1, strumdown1 || mouse_over == 5 ? COLOR_STRUM_DN : COLOR_STRUM_UP);
		DrawRectangleRec(iStrum2, strumdown2 || mouse_over == 6 ? COLOR_STRUM_DN : COLOR_STRUM_UP);

		// green
		DrawRectangleRec(oFret0, COLOR_LANE0_BG);
		DrawRectangleRec(iFret0, keydn[0] || mouse_over == 0 ? COLOR_LANE0_DN : COLOR_LANE0_UP);

		// red
		DrawRectangleRec(oFret1, COLOR_LANE1_BG);
		DrawRectangleRec(iFret1, keydn[1] || mouse_over == 1 ? COLOR_LANE1_DN : COLOR_LANE1_UP);

		// yellow
		DrawRectangleRec(oFret2, COLOR_LANE2_BG);
		DrawRectangleRec(iFret2, keydn[2] || mouse_over == 2 ? COLOR_LANE2_DN : COLOR_LANE2_UP);

		// blue
		DrawRectangleRec(oFret3, COLOR_LANE3_BG);
		DrawRectangleRec(iFret3, keydn[3] || mouse_over == 3 ? COLOR_LANE3_DN : COLOR_LANE3_UP);

		// orange
		DrawRectangleRec(oFret4, COLOR_LANE4_BG);
		DrawRectangleRec(iFret4, keydn[4] || mouse_over == 4 ? COLOR_LANE4_DN : COLOR_LANE4_UP);

		if (mouse_over < 7) {
			i32 vkey;

			char keywait_msg[32];
			sprintf(keywait_msg, "Waiting for key: %c",
				mouse_over < 5 ? '0' + mouse_over :
				mouse_over == 5 ? 'U' : 'D'
			);

			const i32 text_width = FONT_SIZE == 20 ? 178 : MeasureText(keywait_msg, FONT_SIZE);

			if (debug)
				printf(keywait_msg);

			DrawText(keywait_msg, (cur_scrwidth - text_width) >> 1, (cur_scrheight - FONT_SIZE) >> 1, FONT_SIZE, LIGHTGRAY);
			EndDrawing();

			// wait for all keys to be released
			while (keypressed(VK_LBUTTON))
				Sleep(1);

			while (true) {
				for (u8 vk = 1; vk != 0; vk++)
					if (keypressed(vk)) {
						vkey = vk;
						goto vkey_found;
					}

				Sleep(1);
			}

		vkey_found:
			if (debug)
				printf(" - 0x%02x\n", vkey);

			if (mouse_over < 5)
				lanes[mouse_over].vkey = vkey;
			else if (mouse_over == 5)
				strum1.vkey = vkey;
			else
				strum2.vkey = vkey;

			continue; // main loop
		}

		// move down any existing note rectangles down and render them.
		for (u8 i = 0; i < 5; i++) {
			Lane *lane = lanes + i;

			for (u8 j = 0; j < lane->n; j++) {
				// if it was the most recent rectangle, don't touch it.
				// if the rectangle has left the screen,
				// copy in the last rectangle into the current slot to keep contiguousness.
				if ((lane->recs[j].y += move_diff) > cur_scrheight)
					if (--lane->n != 0) {
						if (lane->mr == lane->n)
							lane->mr = j;

						lane->recs[j] = lane->recs[lane->n];
					}

				if (j != lane->mr)
					// the most recent note is rendered in the next loop.
					DrawRectangleRec(lane->recs[j], lane->color);
			}
		}

		for (u8 i = 0; i < 5; i++) {
			Lane *lane = lanes + i;

			if (keydn[i]) {
				if (lane->keydown) {
					// extend the most recent existing note
					lane->recs[lane->mr].y = note_y;
					lane->recs[lane->mr].height += move_diff;
				} else {
					// spawn a new rectangle.
					Rectangle note = {
						.x = x0 + lane_wd*i,
						.y = note_y,
						.width = lane_wd,
						.height = speed / fps
					};

					DrawRectangleRec(note, lane->color);
					if (lane->n == 255)
						lane->recs[lane->mr] = note;
					else {
						lane->recs[lane->n] = note;
						lane->mr = lane->n++;
					}
				}
			} // if (lanes[0].keydown)

			if (lane->n)
				DrawRectangleRec(lane->recs[lane->mr], lane->color);
		}

		// print the FPS over everything else.
		if (debug) {
			sprintf(fps_str, "FPS: %d", fps);
			sprintf(objs_str, "objs: %u (15 %u %u %u %u %u)",
				15 + lanes[0].n + lanes[1].n + lanes[2].n + lanes[3].n + lanes[4].n,
				lanes[0].n, lanes[1].n, lanes[2].n, lanes[3].n, lanes[4].n);

			DrawText(fps_str, FONT_SIZE >> 1, cur_scrheight - (5*FONT_SIZE >> 2), FONT_SIZE, LIGHTGRAY);
			DrawText(objs_str, FONT_SIZE >> 1, cur_scrheight - (5*FONT_SIZE >> 1), FONT_SIZE, LIGHTGRAY);
		}

		EndDrawing();
	}


	if (save_cfgfile) {
		Vector2 tmp  = GetWindowPosition();
		window_pos.x = tmp.x;
		window_pos.y = tmp.y;
	}
	CloseWindow();

	if (!save_cfgfile)
		goto done;

	if (cfgfile == NULL) {
		if ((cfgfile = fopen(CFGFILE, "w")) == NULL) {
			warning("config file could not be opened and did not exist when program launched.");
			goto done;
		}
	}
	else
		freopen(CFGFILE, "w", cfgfile); // truncate.

	fprintf(cfgfile,
		"# boolean options\n"
		"debug: %s\n"
		"vsync: %s\n"
		"top: %s\n"
		"borderless: %s\n"
		"transparent: %s\n"
		"fullscreen: %s\n"
		"\n"
		"# window positioning options\n"
		"pos: %d,%d\n"
		"size: %u,%u\n"
		"\n"
		"# integer options\n"
		"speed: %u\n"
		"fps: %u\n"
		"\n"
		"# keybinds (lane, virtual key code)\n",
		BOOL_STR(debug),
		BOOL_STR(flags & FLAG_VSYNC_HINT),
		BOOL_STR(flags & FLAG_WINDOW_TOPMOST),
		BOOL_STR(flags & FLAG_WINDOW_UNDECORATED),
		BOOL_STR(flags & FLAG_WINDOW_TRANSPARENT),
		BOOL_STR(flags & FLAG_WINDOW_MAXIMIZED),

		window_pos.x, window_pos.y,
		cur_scrwidth, cur_scrheight,

		(unsigned) speed,
		fps_tgt
	);

	{
		// "key: 0,0x%02x\n"
		//                 0   1   2   3   4   5   6   7   8   9   10  11  12  13   14
		char tmp_str[] = {'k','e','y',':',' ','0',',','0','x','%','0','2','x','\n','\0'};

		for (u8 i = 0; i < 5; i++) {
			tmp_str[5] = '0' + i;

			// option 1
			if (lanes[i].vkey < 'A' || lanes[i].vkey > 'Z') {
				tmp_str[7]  = '0';
				tmp_str[8]  = 'x';
				tmp_str[9]  = '%';
				tmp_str[10] = '0';
				tmp_str[11] = '2';
				tmp_str[12] = 'x';
			}
			else {
				tmp_str[7]  = '%';
				tmp_str[8]  = 'c';
				tmp_str[9]  = '\n';
				tmp_str[10] = '\0';
			}

			fprintf(cfgfile, tmp_str, lanes[i].vkey);
		}

		tmp_str[5] = 'U';
		if (strum1.vkey < 'A' || strum1.vkey > 'Z') {
			tmp_str[7]  = '0';
			tmp_str[8]  = 'x';
			tmp_str[9]  = '%';
			tmp_str[10] = '0';
			tmp_str[11] = '2';
			tmp_str[12] = 'x';
		}
		else {
			tmp_str[7]  = '%';
			tmp_str[8]  = 'c';
			tmp_str[9]  = '\n';
			tmp_str[10] = '\0';
		}
		fprintf(cfgfile, tmp_str, strum1.vkey);

		tmp_str[5] = 'D';
		if (strum2.vkey < 'A' || strum2.vkey > 'Z') {
			tmp_str[7]  = '0';
			tmp_str[8]  = 'x';
			tmp_str[9]  = '%';
			tmp_str[10] = '0';
			tmp_str[11] = '2';
			tmp_str[12] = 'x';
		}
		else {
			tmp_str[7]  = '%';
			tmp_str[8]  = 'c';
			tmp_str[9]  = '\n';
			tmp_str[10] = '\0';
		}
		fprintf(cfgfile, tmp_str, strum2.vkey);
	}

done:
	if (debug)
		puts("\nexiting.");

	exit(0);
}

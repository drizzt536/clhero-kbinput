// compile instructions: make all

/*
 * exit codes:
 *  - 0: success
 *  - 1: generic error. (look at exit message)
 *  - 2: out of memory
 *
 * relevant links:
 *  - https://github.com/raphaelgoulart/ya_inputdisplay
 *  - https://github.com/raysan5/raylib
*/

#ifndef SAVEFILE
	#define SAVEFILE "./clhero-kbinput.save"
#endif


// TODO: remember the previous size and position of the window through program exits.
//       probably use a config text file for easy editing.
// TODO: consider looking into <xinput.h> or hidapi for controller support?
// TODO: fix bugs:
//       - when you make a note, the msot recent not disappears for one frame.
//       - sometimes the notes will jump back and forth by a bit.
// TODO: make it save your data across exits without using the windows registry.
// TODO: have a way to change the colors of everything.
//       the outer fret things should be around 112/159 times the inner.
//       the note colors should either be custom, or derived from the fret colors
// TODO: make it so it closes other instances of the program if you open a new one.
// TODO: finish the `kpct` (key press count) stuff.
// TODO: actually handle bind changes after you click on a fret.
// TODO: add default keybinds to the help text?
//       at least mention that backspace removes notes and resets input counts.


#ifndef _WIN64
	#error "this program only works on 64-bit windows."
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
#define VK_BACK 0x08
#define VK_SPACE 0x20
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
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
#define SEEK_SET 0
#define I32MAX 2147483647
typedef struct { void *_Placeholder; } FILE;
i32 printf(const char *fmt, ...);
i32 vsnprintf(char *str, u64 n, const char *format, __builtin_va_list args);
i32 sprintf(char *str, const char *format, ...);
i32 puts(const char *str);
i32 putchar(i32 ch);
i32 _getch_nolock(void);
i32 strcmp(const char *str1, const char *str2);
FILE *__acrt_iob_func(u32 index);
u32 strtoul(const char *str, char **endptr, i32 base);
double strtod(const char *str, char **endptr);
int rand(void);
void __attribute__((noreturn)) exit(i32 exitcode);
void _crt_atexit(void (*callback)(void));
FILE *fopen(const char *filename, const char * mode);
i32 _fseeki64_nolock(FILE *stream, i64 offset, i32 origin);
i64 _ftelli64_nolock(FILE *stream);
i32 _fclose_nolock(FILE *stream);
FILE *freopen(const char *filename, const char *mode, FILE *stream);
i32 fputs(const char *str, FILE *stream);
void *malloc(u64 size);
void free(void *ptr);
float roundf(float x);
#define fseek _fseeki64_nolock
#define ftell _ftelli64_nolock
#define fclose _fclose_nolock
#define _getch _getch_nolock
#define atexit _crt_atexit

#include "raylib.h"

#define _AF_ATTR(x) __attribute__((cleanup(cleanup_##x)))
#define AUTO_FREE(x) _AF_ATTR(x) x
#define AF_char      _AF_ATTR(array) char
#define AF_LaneArray _AF_ATTR(LaneArray) Lane
#define AF_FILE      AUTO_FREE(FILE) // should be a `FILE *` variable and not just `FILE`
#define FORCE_INLINE static inline __attribute__((always_inline))


#define EXIT_OOM 2

// STR_IN only works up to four options in the set.
#define streq(s1, s2) (strcmp(s1, s2) == 0)
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

#define _DrawRectangleRec(r, c) { DrawRectangleRec(r, c); puts("drawing rectangle"); }

// variables that are used in more than one function.
static const char *const cons_title = "(terminal) Guitar Hero Keyboard Input Displayer";
static FILE *stdin, *stdout, *stderr, *savefile = NULL;
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
	note_y = 0, // note spawn height
	x0 = 0, x1, x2, x3, x4,
	lane_wd = 0; // lane width

FORCE_INLINE void cleanup_array(const void *p) {
	free(* (void **) p);
}

FORCE_INLINE void cleanup_FILE(FILE *const *fp) {
	fclose(*fp);
}

FORCE_INLINE void cleanup_LaneArray(Lane *const *p) {
	Lane *x = *(Lane **) p;

	while (x->recs != NULL) {
		free(x->recs);
		x++;
	}
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

	puts("ClHero-KbInput v0.1");

	debug = true;
}

void exit_handler(void) {
	if (savefile != NULL)
		fclose(savefile);

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

FORCE_INLINE void rescale_window(void) {
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

void fatal(i32 exitcode, const char *fmt, ...) {
	if (exitcode == EXIT_OOM)
		goto fallback_oom;

	// 256 is a guess at how long error messages should cap out at.
	char *const str = malloc(256);
	if (str == NULL)
		goto fallback_oom;

	__builtin_va_list args;
	__builtin_va_start(args, fmt);
	vsnprintf(str, 256, fmt, args);
	__builtin_va_end(args);

	if (debug)
		printf("\e[31m%s\e[0m\n", str);
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

void init_args(void); // exported from assembly. init-crt.o

extern i32 __stdcall WinMainCRTStartup(void) {
	i32 argc;
	char **argv;

	asm volatile (
		"call init_args\n\t"
		"movl %%edi, %0\n\t"
		"movq %%rsi, %1"
		: "=r"(argc), "=r"(argv)
		:: "rdi", "rsi", "memory"
	);

	stdin  = __acrt_iob_func(0);
	stdout = __acrt_iob_func(1);
	stderr = __acrt_iob_func(2);
	/////////////////////////////////////////////

	atexit(&exit_handler);

	if (argc > 1) {
		// parse arguments that have to come first.
		if (streq(argv[1], "--debug"))
			enable_debug();
		else if (streq(argv[1], "--version")) {
			enable_debug();
			exit(0);
		}
		else if (STR_IN(argv[1], "--help", "-h", "-?")) {
			enable_debug();

			puts(
				"\noptions:"
				"\n    --help, -h, -?  print this message and exit. must be the first argument."
				"\n    --version       print the version and exit."
				"\n    --debug         allocate a console for debug information. must be the first argument."
				"\n    --fps FPS       specify a target fps for raylib. must be a non-negative integer. pass 0 or \"uncapped\" to remove the fps cap."
				"\n    --speed PPS     specify a note speed in pixels per second. any double-precision floating-point number"
				"\n    --top           display window always on top."
				"\n    --undecorated   display without the toolbar buttons at the top."
				"\n    --transparent   display with a transparent window."
				"\n    --fullscreen    display the window in fullscreen mode."
				"\n    --vsync         try to enable vsync. only works if the fps is greater than the refresh rate."
				"\n    --highdpi       use for high dpi displays? (possible broken)"
				"\n" // double newline.
			);

			exit(0);
		}
	}

	double speed = 300; // pixels per second
	i32 fps_tgt = 1000, // 1000 fps target by default.
		flags = FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN;

	bool transparent = false,
		fullscreen = false;

	if (debug) {
		printf("argument dump: argc=%d, argv=0x%p,\n", argc, argv);

		for (i32 i = 0; i < argc; i++)
			printf("    argv[%d] = '%s'\n", i, argv[i]);
	}

	// parse CLI arguments.
	for (i32 i = 1 + debug; i < argc; i++) {
		if (streq(argv[i], "--fps")) {
			i++;

			if (i == argc)
				fatal(1, "`--fps` given without a value.");

			if (streq(argv[i], "uncapped")) {
				fps_tgt = 0;
				continue;
			}

			char *end;
			fps_tgt = strtoul(argv[i], &end, 0);

			if (*end != '\0' || fps_tgt < 0)
				fatal(1, "`--fps` given with an invalid value `%s`.", argv[i]);
		}
		else if (streq(argv[i], "--speed")) {
			i++;

			if (i == argc)
				fatal(1, "`--speed` given without a value.");

			char *end;
			speed = strtod(argv[i], &end);

			if (*end != '\0' || speed < 0)
				fatal(1, "`--speed` given with an invalid value `%s`.", argv[i]);
		}
		else if (streq(argv[i], "--top"))
			flags |= FLAG_WINDOW_TOPMOST;
		else if (streq(argv[i], "--transparent")) {
			transparent = true;
			flags |= FLAG_WINDOW_TRANSPARENT;
		}
		else if (streq(argv[i], "--undecorated"))
			flags |= FLAG_WINDOW_UNDECORATED;
		else if (streq(argv[i], "--vsync"))
			flags |= FLAG_VSYNC_HINT;
		else if (streq(argv[i], "--highdpi"))
			flags |= FLAG_WINDOW_HIGHDPI;
		else if (streq(argv[i], "--fullscreen")) {
			// FLAG_BORDERLESS_WINDOWED_MODE seems to have the same effect
			flags |= FLAG_WINDOW_MAXIMIZED;
			fullscreen = true;
		}
		else
			fatal(1, "unknown argument `%s` at position %d.\nuse `--help` for help.", argv[i], i);
	}

	// savefile = fopen(SAVEFILE, "a+"); // if it exists
	// if (savefile == NULL)

	// fseek(savefile, 0, SEEK_SET);
	// fputs("asdf\n", savefile);
	// fclose(savefile);

	if (debug) printf(
		"using fps target: %d\n"
		"using speed: %lg\n"
		"----------------------------------------------------\n",
		fps_tgt, speed
	);
	// end of CLI setup

	KeyData strum1 = {VK_UP, false}, strum2 = {VK_LEFT, false};

	u8 mouse_over = 0;

	float frame_time;
	double move_diff;
	u32 fps;

	bool keydn[5] = {0};
	bool strumdown1 = false,
		strumdown2 = false;

	Vector2 mousepos;

	// these two are only used in debug mode.
	AF_char *fps_str = malloc(16); // 1 + strlen("FPS: 2147483647")
	AF_char *objs_str = malloc(36); // 1 + strlen("objs: 1290 (15 255 255 255 255 255)")

	// NOTE: these are never freed because they are used until exit.
	// NOTE: 16 == sizeof(Rectangle)
	AF_LaneArray *const lanes = (Lane [6]) {
		{ .recs = malloc(256 * sizeof(Rectangle)), .color = COLOR_LANE0_NT, .vkey = 'S' },
		{ .recs = malloc(256 * sizeof(Rectangle)), .color = COLOR_LANE1_NT, .vkey = 'D' },
		{ .recs = malloc(256 * sizeof(Rectangle)), .color = COLOR_LANE2_NT, .vkey = 'F' },
		{ .recs = malloc(256 * sizeof(Rectangle)), .color = COLOR_LANE3_NT, .vkey = 'G' },
		{ .recs = malloc(256 * sizeof(Rectangle)), .color = COLOR_LANE4_NT, .vkey = 'H' },
		{ .recs = NULL }
	};

	// lane[4].recs == NULL implies lane[0].recs through lane[3].recs are also NULL.
	// because the allocations are the same size, with no possible `free`s in between.
	// the fps_str allocation is smaller and at the start, so that check isn't necessary either
	// also it is the start of the program, so why would it be out of memory
	if (/*fps_str == NULL ||*/ lanes[4].recs == NULL)
		fatal(EXIT_OOM, NULL);

	SetConfigFlags(flags);
	InitWindow(fullscreen ? 0 : 450, fullscreen ? 0 : 800, cons_title + 11); // 11 == strlen("(terminal) ")
	SetTargetFPS(fps_tgt);

	// NOTE: 300 is picked so `scale` is never less than 1.
	//       180 is so it always shows the frets and a little bit more past that.
	//       if these numbers change, they also need to change in `rescale_window`.
	SetWindowMinSize(300, 180);

	while (!WindowShouldClose()) {
		fps = GetFPS();
		cur_scrheight = GetRenderHeight();
		mousepos = GetMousePosition();
		frame_time = GetFrameTime();
		move_diff = speed * frame_time;

		if ((cur_scrwidth = GetRenderWidth()) != prev_scrwidth) {
			float prev_scale = scale;
			rescale_window();

			float factor = scale / prev_scale;
			for (u8 i = 0; i < 5; i++) {
				Lane *lane = lanes + i;
				Rectangle *recs = lane->recs;

				for (u8 j = 0; j < lane->n; j++) {
					recs[j].x *= factor;
					recs[j].y *= factor;
					recs[j].width *= factor;
					recs[j].height *= factor;
				}
			}
		}

		strum1.down = strumdown1; strumdown1 = GetAsyncKeyState(strum1.vkey) & 0x8000;
		strum2.down = strumdown2; strumdown2 = GetAsyncKeyState(strum2.vkey) & 0x8000;	
		for (u8 i = 0; i < 5; i++) {
			lanes[i].keydown = keydn[i];

			if ((keydn[i] = GetAsyncKeyState(lanes[i].vkey) & 0x8000) && !lanes[i].keydown)
				lanes[i].kpct++;
		}

		// delete all the notes and set the key counts to zero.
		if (GetAsyncKeyState(VK_BACK) & 0x8000)
			for (u8 i = 0; i < 5; i++)
				lanes[i].n = lanes[i].mr = lanes[i].kpct = lanes[i].keydown = 0;


		mouse_over = 0; // default value
		(void) mouse_over;
		// NOTE: mouse_over only matters if the butten was just clicked.

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			if      (CheckCollisionPointRec(mousepos, oFret0)) mouse_over = 1;
			else if (CheckCollisionPointRec(mousepos, oFret1)) mouse_over = 2;
			else if (CheckCollisionPointRec(mousepos, oFret2)) mouse_over = 3;
			else if (CheckCollisionPointRec(mousepos, oFret3)) mouse_over = 4;
			else if (CheckCollisionPointRec(mousepos, oFret4)) mouse_over = 5;
			else if (CheckCollisionPointRec(mousepos, oStrum1)) mouse_over = 6;
			else if (CheckCollisionPointRec(mousepos, oStrum2)) mouse_over = 7;
		}

		// if mouse_over

		BeginDrawing();
		ClearBackground(transparent ? BLANK : BLACK);

		// purple
		DrawRectangleRec(oStrum, COLOR_STRUM_BG);
		DrawRectangleRec(iStrum1, strumdown1 ? COLOR_STRUM_DN : COLOR_STRUM_UP);
		DrawRectangleRec(iStrum2, strumdown2 ? COLOR_STRUM_DN : COLOR_STRUM_UP);

		// green
		DrawRectangleRec(oFret0, COLOR_LANE0_BG);
		DrawRectangleRec(iFret0, keydn[0] ? COLOR_LANE0_DN : COLOR_LANE0_UP);

		// red
		DrawRectangleRec(oFret1, COLOR_LANE1_BG);
		DrawRectangleRec(iFret1, keydn[1] ? COLOR_LANE1_DN : COLOR_LANE1_UP);

		// yellow
		DrawRectangleRec(oFret2, COLOR_LANE2_BG);
		DrawRectangleRec(iFret2, keydn[2] ? COLOR_LANE2_DN : COLOR_LANE2_UP);

		// blue
		DrawRectangleRec(oFret3, COLOR_LANE3_BG);
		DrawRectangleRec(iFret3, keydn[3] ? COLOR_LANE3_DN : COLOR_LANE3_UP);

		// orange
		DrawRectangleRec(oFret4, COLOR_LANE4_BG);
		DrawRectangleRec(iFret4, keydn[4] ? COLOR_LANE4_DN : COLOR_LANE4_UP);

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
				// c=current frame, p=previous frame
				// printf(" key %d c=%d,p=%d", i, keydn[i], lane->keydown);
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
			// sprintf(objs_str, "objs: %u", 15 + lanes[0].n + lanes[1].n + lanes[2].n + lanes[3].n + lanes[4].n);

			DrawText(fps_str, 10, cur_scrheight - 25, 20, LIGHTGRAY);
			DrawText(objs_str, 10, cur_scrheight - 50, 20, LIGHTGRAY);
			
		}

		EndDrawing();
	}

	CloseWindow();

	// freopen(SAVEFILE, "w", savefile); // truncate.
	puts("\nexiting.");

	exit(0);
}

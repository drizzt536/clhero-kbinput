Similar to [this](https://github.com/raphaelgoulart/ya_inputdisplay) for Clone Hero input displaying, but intended for keyboard input instead of controller input. The other program doesn't (and cannot) accept keyboard input unless it is the focused program.

This version should also produce executables magnitudes smaller since it doesn't use Godot, or even the bulk of a C runtime.

For build requirements, refer to the comments at the top of `Makefile`. Once they are fulfilled, run `make` to build.

For a list of updates planned, look at the TODO comments at the top of `clhero-kbinput.c`.

Currently, the program's behavior can only be customized through command-line arguments.

If the program looks super jumpy and flashy, it is likely an issue with the FPS; the program looks much better the higher the FPS.
 - Open the program in debug mode to check the FPS.
 - If the FPS drops when the program leaves focus, you probably need to reconfigure your PC in the NVIDIA control panel to stop limiting FPS on background processes.
 - If it is always low, you may need to change the FPS limit in the program configuration, or turn off vsync.

For any complaints or suggestions, submit a GitHub issue.
You may also want to email me at djanusch2007@gmail.com, so I am more likely to see it.

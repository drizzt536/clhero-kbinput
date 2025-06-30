Similar to [this](https://github.com/raphaelgoulart/ya_inputdisplay) for Clone Hero input displaying, but intended for keyboard input instead of controller input. The other program doesn't (and cannot) accept keyboard input unless it is the focused program.

This version should also produce executables magnitudes smaller since it doesn't use Godot, or even the bulk of a C runtime.

For build requirements, refer to the comments at the top of `Makefile`. Once they are fulfilled, run `make` to build.

For a list of updates planned, look at the TODO comments at the top of `clhero-kbinput.c`.

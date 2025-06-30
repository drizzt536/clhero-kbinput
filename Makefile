# build requirements:
	# nasm
	# MinGW: GCC, ld, make, unzip, which
	# MinGW or MSYS2: sed, wget, rm
	# Windows 10+

# I recommend downloading w64devkit-x64-[VERSION].7z.exe
# from https://github.com/skeeto/w64devkit/releases/latest
# and t hen adding it to path.

override SETUP := init-crt
override BASE := clhero-kbinput

# disables stripping and section removal, and reduces optimization.
DEBUG ?= FALSE

# this only matters if you don't already have raylib installed.
RAYLIB_VER := LATEST

ifeq ($(DEBUG), TRUE)
	CFLAGS := -Og
	LDFLAGS :=
else
	override LDFLAGS := --gc-sections -s
	override CFLAGS := -O3
endif

all: $(SETUP).o $(BASE).o main.exe

raylib:
	@echo "# downloading dependency: raylib"
	@rm -rf raylib # remove the folder in case you do have it.
ifeq ($(RAYLIB_VER),LATEST)
	version=$$(curl -fsi https://github.com/raysan5/raylib/releases/latest | findstr ^Location | sed -E 's:.+/::'); \
	wget --no-cache --quiet https://github.com/raysan5/raylib/releases/download/$${version}/raylib-$${version}_win64_mingw-w64.zip; \
	unzip -qjd raylib raylib-$${version}_win64_mingw-w64.zip -x *dll* *E* *math.h *gl.h; \
	rm raylib-$${version}_win64_mingw-w64.zip
else
	wget --no-cache --quiet "https://github.com/raysan5/raylib/releases/download/$(RAYLIB_VER)/raylib-$(RAYLIB_VER)_win64_mingw-w64.zip"
	unzip -qjd raylib raylib-$(RAYLIB_VER)_win64_mingw-w64.zip -x *dll* *E* *math.h *gl.h
	rm raylib-$(RAYLIB_VER)_win64_mingw-w64.zip
endif
	@echo "" # empty line.

raylib/libraylib.a raylib/raylib.h: raylib
	@# assumes that if you don't have one, you don't have any.

$(SETUP).o: $(SETUP).nasm
	nasm -fwin64 $(SETUP).nasm -o $(SETUP).o

$(BASE).o: $(BASE).c raylib/raylib.h
	gcc -Wall -Wextra -Werror $(CFLAGS) -Iraylib -c $(BASE).c

main.exe: $(SETUP).o $(BASE).o raylib/libraylib.a
	@# the -L folder is so it can file -lgcc. there still doesn't have the startup files though.
	@# ld doesn't seem to cut out all of the unused functions, so it could make sense to recompile raylib locally.
	ld $(LDFLAGS) --subsystem windows $(SETUP).o $(BASE).o -Lraylib -L"$(shell which ld.exe)/../../lib/gcc/x86_64-w64-mingw32/15.1.0" -lraylib -lmingwex -lgcc -lgdi32 -lucrtbase -lkernel32 -lshell32 -luser32 -lwinmm -o $(BASE).exe

clean:
	rm -f *.o

distclean:
	rm -f *.o *.exe


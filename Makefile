SHELL = /bin/sh

SRC_FILES := $(wildcard src/*.cpp)
HEADERS := $(wildcard src/*.h)
HOMEBREW_DEPENDENCIES := fftw portaudio
MACPORTS_DEPENDENCIES := fftw-3 portaudio

CPPFLAGS += -std=c++11 \
	-O3 -ffast-math -flto \
	-Wall -Wextra -pedantic -Wno-deprecated \
	-Wfloat-conversion -Wconversion -Wsign-conversion \
	-Wshorten-64-to-32 -Wdouble-promotion

build/main : environment $(SRC_FILES) $(HEADERS)
	mkdir -p build;
	g++ $(CPPFLAGS) $(SRC_FILES) \
		-framework OpenGL \
		-framework GLUT \
		-lportaudio \
		-lfftw3 \
		-o $@;

.PHONY : environment
environment :
	@ if which brew >/dev/null; then \
		if ! brew list $(HOMEBREW_DEPENDENCIES) >/dev/null 2>&1; then \
			brew install $(HOMEBREW_DEPENDENCIES); \
		fi; \
	elif which port >/dev/null; then \
		port install $(MACPORTS_DEPENDENCIES); \
	fi;

.PHONY : clean
clean :
	rm build/main || true;

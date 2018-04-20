# Echolocator

This project demonstrates using a computer speaker and microphone to
determine distances to objects using echolocation.

Note that the distance found is a crude measure; differences in
computer hardware and speaker placement make accurate distance
detection impossible without prior calibration. The true distance to
an object will be `true_distance = a * measured_distance + b`, where
`a` is an unknown constant (close to 1) controlled by the distance
between the microphone and the speaker, and `b` is another unknown
constant controlled by the time taken between generating a signal and
the speaker producing it, plus the delay between the microphone
receiving a singal and the program processing it. There will also be
small errors due to slight differences in the speed of sound due to,
for example, air pressure.

## Setup

The makefile will attempt to install dependencies in known
environments:

```shell
make environment
```

Alternatively, you can install the dependencies yourself.

### MacOS with Homebrew

```shell
brew install fftw portaudio
```

### MacOS with MacPorts

```shell
port install fftw-3 portaudio
```

### General

This project will build and run on unix-based systems, and possibly
Windows too (all dependencies are cross-platform).

The required libraries are:
* [the Fastest Fourier Transform in the West](http://www.fftw.org/)
* [Portaudio](http://www.portaudio.com/)
* OpenGL and GLUT

## Build and Run

```shell
make
build/main
```

The main application window will open and the speakers will begin
producing rapid clicks. After a few seconds of calibration, the display
will show vertical bars representing echos received. This typically
involves one strong bar (for the sound travelling directly from the
speaker to the microphone), then a cluster of fuzzy bars (various
reflections from the computer itself) then an emptier area. You can try
moving flat objects (card, a hand, etc.) near the computer to see them
appear in this area.

## Theory

Sadly I can't find the original website which inspired this
implementation, but I have found a page which summarises a lot of
interesting research on how bats use echolocation to determine
position, velocity, approximate shape, and even elevation.
https://www.hscott.net/the-dsp-behind-bat-echolocation/

The general approach used here is to send out a "chirp" sound (a short
tone with rapidly changing pitch), then use fourier transforms to
deconvolve the returned audio. By deconvolving using the original
chirp, it is possible to detect reflections with high accuracy while
dismissing background noise. This allows a map of distance to
reflection intensity to be produced (where time-since-chirp is used
as a measure of the distance), and this map is displayed to the user.

## Structure

The vague structure of this project is:
* `chirps.hpp`: contains signal generators (tone, chirp and repeating
  chirp)
* `fourier.hpp`: simple object-based wrapper around FFTW
* `audio.cpp`: the main logic for the echolocation process
* `renderer.cpp`: a simple wrapper around OpenGL / GLUT for
  displaying bitmap data
* `main.cpp`: main entrypoint for the program and orchastration

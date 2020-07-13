# Audio Mixing and Input/Output

You reached the homepage of the AMIO package: a Python package for
Audio Mixing and Input/Output.

**AMIO is still under early development**

This means the API will change, there are basic functionalities missing,
there is an undesired tight coupling with the JACK driver, etc. Everything
here in this README states the intended state of the library when it
reaches certain quality. When this happens, this notice will be removed.

## About

This package provides a way to capture input audio and playback output audio
in Python. The distinguishing feature is the API design.

When starting I/O on an interface, a background thread is created to manage
the actual I/O. The application code retrieves the received input audio,
and schedules so called playspecs to be played back.

A playspec consists of a number of audio clips, each starting at a given
point in time and possibly cropped, and with a given gain for the left
and right channels.

All timestamps are expressed in the number of frames, giving a frame-exact
precision.

Currently, only JACK Audio Connection Kit on Linux is supported as
the input/output interface. However, I expect that it will compile and run
(maybe with small modifications) on any platform with JACK. Support for ALSA
is planned. Support for any other sound system is welcome as a pull request.

Currently only stereo (2 channel) input/output is supported. Support for
other number of channels is not planned. The audio clips can be 1-
or 2-channel.

All audio clips used in a playspec must have a frame rate matched to
the interface frame rate. The AudioClip class provides a way to do frame
rate conversion.

## Glossary

The convention is followed throughout the package sources that a "sample" means
a single-channel audio sample, while "frame" means a possibly-multichannel
set of samples at a given point in time. Hence "frame rate" is used to mean
the frequency at which frames are produced or consumed. For instance, the CD
quality is: frame rate of 44100 Hz, 2 channels, 16-bit samples.

## Third-party code

Ring buffer implementation comes from the PortAudio project. It's in the files

 * amio/pa_ringbuffer.h
 * amio/pa_ringbuffer.c
 * amio/pa_memorybarrier.h

copied directly from PortAudio source code (from src/common)
at Git tag pa_stable_v190600_20161030, which is the recommended stable release
of PortAudio while I'm writing this.

## Author

Michał Szymański, 2019-2020

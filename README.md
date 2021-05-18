# Audio Mixing and Input/Output

You reached the homepage of the AMIO package: a Python package for
Audio Mixing and Input/Output.

AMIO serves primarily as the audio abstraction for
[Manokee](https://github.com/smiszym/manokee) and the roadmap of the project is
to write features that Manokee needs; at least for now.

**AMIO is still under early development.** This means the API will change,
there are basic functionalities missing, there is an undesired tight coupling
with the JACK driver, etc.

## About

This package provides a way to capture input audio and playback output audio
in Python. It's designed with the following as goals:

* Frame-accurate timing, both for playback and recording.
* Representing audio data as NumPy arrays.
* Low latency capture and reliability thanks to a dedicated non-Python thread
  for audio I/O (currently, you can't easily send input back to output, but you
  can analyze the input with low latency)
* Ability to specify the output stream as a _playspec_ (playback
  specification). This way mixing logic is handled by AMIO and not needed
  in the application.

## Installation

An sdist is available in PyPI. To compile and use it, you need to install
several dependencies first. On RPM-based distributions (CentOS, Fedora),
they can be installed with:

```bash
yum install -y jack-audio-connection-kit-devel libsndfile swig
```

Then, simply do:

```bash
pip install amio
```

pip will download the sdist, build it and install it.

## How to use AMIO

In order to play back and capture sound, first create an AMIO interface with
`amio.create_io_interface`. Then await `init` on this interface. At this
moment, a background thread is created to manage the audio input/output.

In order to capture audio on the interface, set `input_chunk_callback` property
on the interface object you created. The callback needs to be a function that
accepts a single argument of type `InputAudioChunk`. In regular intervals, this
callback will get called with a new chunk of input audio data.

In order to play back audio on the interface, create a playspec and call
`schedule_playspec_change` on the interface, supplying the playspec
as an argument. A playspec is a list of `amio.PlayspecEntry` objects.

Every playspec entry is a single (possibly cropped) audio clip starting
at a given point in time, with a specified gain for the left and right channels.

## Limitations

Currently, only JACK Audio Connection Kit on Linux is supported
as the input/output interface. However, I expect that it will compile and run
(maybe with small modifications) on any platform with JACK. Support for ALSA
is planned. Support for any other sound system is welcome as a pull request.

Currently only stereo (2 channel) input/output is supported. Support for other
number of channels is not planned in the near future. The audio clips can be
mono or stereo.

All audio clips used in a playspec must have a frame rate matched to the
interface frame rate. If frame rate conversion is required, `AudioClip` class
provides a way to do it.

The memory management is inefficient at the moment. The audio data is being
copied too much. There is a plan to improve it, but it's not going to happen
in the near future probably.

## Third-party code

Ring buffer implementation comes from the PortAudio project. It's in the files

 * amio/pa_ringbuffer.h
 * amio/pa_ringbuffer.c
 * amio/pa_memorybarrier.h

copied directly from PortAudio source code (from src/common)
at Git tag pa_stable_v190600_20161030, which is the recommended stable release
of PortAudio while I'm writing this.

## Contributing

Any contributions to this project are very much welcome. Please open an issue
or discussion if you want to contribute code, or have an idea for a feature,
in order to discuss the implementation details before starting work.

## Author

Michał Szymański, 2019-2021

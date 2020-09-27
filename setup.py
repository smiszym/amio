#!/usr/bin/env python3

from setuptools import setup, Extension

import codecs
import os.path


def read(rel_path):
    here = os.path.abspath(os.path.dirname(__file__))
    with codecs.open(os.path.join(here, rel_path), 'r') as fp:
        return fp.read()


def get_version(rel_path):
    for line in read(rel_path).splitlines():
        if line.startswith('__version__'):
            delim = '"' if '"' in line else "'"
            return line.split(delim)[1]
    else:
        raise RuntimeError("Unable to find version string.")


core_module = Extension(
    'amio/_core',
    sources=[
        'amio/core.i',
        'amio/audio_clip.c',
        'amio/communication.c',
        'amio/input_chunk.c',
        'amio/interface.c',
        'amio/jack_interface.c',
        'amio/mixer.c',
        'amio/playspec.c',
        'amio/pa_ringbuffer.c',
    ],
    libraries=['jack'],
)

setup(
    name = 'amio',
    version = get_version("amio/__init__.py"),
    author = "Michał Szymański",
    author_email="smiszym@gmail.com",
    description = """Audio Mixing and Input/Output""",
    license="MIT",
    url="https://github.com/smiszym/amio",
    ext_modules = [core_module],
    py_modules = [
        "amio",
        "amio.core",
        "amio.audio_clip",
        "amio.dummy_interface",
        "amio.fader",
        "amio.interface",
        "amio.jack_interface",
        "amio.playspec",
    ],
    requires = [
        "mypy",
    ],
    install_requires = [
        "matplotlib",
        "numpy",
        "soundfile",
    ],
)

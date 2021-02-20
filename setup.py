#!/usr/bin/env python3

from setuptools import setup, Extension

import codecs
import os.path


def read(rel_path):
    here = os.path.abspath(os.path.dirname(__file__))
    with codecs.open(os.path.join(here, rel_path), "r") as fp:
        return fp.read()


def get_version(rel_path):
    for line in read(rel_path).splitlines():
        if line.startswith("__version__"):
            delim = '"' if '"' in line else "'"
            return line.split(delim)[1]
    else:
        raise RuntimeError("Unable to find version string.")


# Switch to True to get unoptimized native code with assertions enabled
debug_variant = False

if debug_variant:
    extra_compile_args = ["-O0"]
    undef_macros = ["NDEBUG"]
else:
    extra_compile_args = []
    undef_macros = []


native_module = Extension(
    "amio._native",
    sources=[
        "amio/native.i",
        "amio/audio_clip.c",
        "amio/communication.c",
        "amio/gc.c",
        "amio/input_chunk.c",
        "amio/interface.c",
        "amio/jack_driver.c",
        "amio/mixer.c",
        "amio/playspec.c",
        "amio/pool.c",
        "amio/pa_ringbuffer.c",
    ],
    libraries=["jack"],
    extra_compile_args=extra_compile_args,
    undef_macros=undef_macros,
)

setup(
    name="amio",
    version=get_version("amio/__init__.py"),
    author="Michał Szymański",
    author_email="smiszym@gmail.com",
    description="""Audio Mixing and Input/Output""",
    license="MIT",
    url="https://github.com/smiszym/amio",
    ext_modules=[native_module],
    packages=["amio"],
    requires=[
        "mypy",
    ],
    install_requires=[
        "matplotlib",
        "numpy",
        "soundfile",
    ],
)

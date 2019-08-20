# Public API
from amio.audio_clip import AudioClip
from amio.fader import factor_to_dB, dB_to_factor, Fader
from amio.playspec import Playspec

import amio.core
from amio.dummy_interface import DummyInterface
from amio.jack_interface import JackInterface


__version__ = "0.1"


def create_io_interface(driver='jack', **kwargs):
    if driver == 'dummy':
        return DummyInterface(**kwargs)
    elif driver == 'jack':
        return JackInterface()
    else:
        raise NotImplementedError('No such AMIO driver')

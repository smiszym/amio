from amio.audio_clip import AudioClip, InputAudioChunk
from amio.fader import factor_to_dB, dB_to_factor, Fader
from amio.playspec import Playspec, PlayspecEntry

from amio.interface import Interface
from amio.dummy_interface import DummyInterface
from amio.native_interface import NativeInterface
from amio.null_interface import NullInterface


__version__ = "0.1.1"


def create_io_interface(driver: str = "jack", **kwargs) -> Interface:
    if driver == "dummy":
        return DummyInterface(**kwargs)
    elif driver == "null":
        return NullInterface(**kwargs)
    elif driver == "jack":
        return NativeInterface()
    else:
        raise NotImplementedError("No such AMIO driver")

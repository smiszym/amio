from amio.audio_clip import InputAudioChunk
from amio.interface import Interface
from datetime import datetime, timedelta
import numpy as np


class NullInterface(Interface):
    """
    Null interface provides silence at the capture stream and discards
    the playback stream.
    """

    chunk_length = 4800  # 0.1 s at 48 kHz

    def __init__(self, frame_rate, starting_time=None):
        self._frame_rate = frame_rate
        self._position = 0
        self._is_transport_rolling = False
        self._playspec = None
        self._closed = False
        self._input_chunk_callback = None
        self._time = starting_time or datetime.now()

    @property
    def input_chunk_callback(self):
        return self._input_chunk_callback

    @input_chunk_callback.setter
    def input_chunk_callback(self, callback):
        self._input_chunk_callback = callback

    def advance_single_chunk_length(self):
        chunk = InputAudioChunk(
            np.zeros((self.chunk_length, 2), np.float32), self._frame_rate,
            self._position, self._is_transport_rolling)
        if self._is_transport_rolling:
            self._position += self.chunk_length
        self._time += timedelta(seconds=(self.chunk_length / self._frame_rate))
        self._input_chunk_callback(chunk)

    def get_current_virtual_time(self):
        return self._time

    def get_frame_rate(self):
        assert not self._closed
        return self._frame_rate

    def get_position(self):
        assert not self._closed
        return self._position

    def set_position(self, position):
        assert not self._closed
        self._position = position

    def is_transport_rolling(self):
        assert not self._closed
        return self._is_transport_rolling

    def set_transport_rolling(self, rolling):
        assert not self._closed
        self._is_transport_rolling = rolling

    def set_current_playspec(self, playspec):
        assert not self._closed
        self._playspec = playspec

    def close(self):
        assert not self._closed
        self._closed = True

    def is_closed(self):
        return self._closed

from amio.audio_clip import InputAudioChunk
from amio.interface import Interface, InputChunkCallback
from amio.playspec import Playspec
from datetime import datetime, timedelta
import numpy as np
from typing import Optional


class NullInterface(Interface):
    """
    Null interface provides silence at the capture stream and discards
    the playback stream.
    """

    chunk_length = 4800  # 0.1 s at 48 kHz

    def __init__(self, frame_rate: float, starting_time: datetime = None):
        super().__init__()
        self._frame_rate = frame_rate
        self._position = 0
        self._is_transport_rolling = False
        self._playspec: Optional[Playspec] = None
        self._closed = False
        self._time = starting_time or datetime.now()

    def advance_single_chunk_length(self) -> None:
        chunk = InputAudioChunk(
            np.zeros((self.chunk_length, 2), np.float32), self._frame_rate,
            self._position, self._is_transport_rolling, self._time)
        if self._is_transport_rolling:
            self._position += self.chunk_length
        self._time += timedelta(seconds=(self.chunk_length / self._frame_rate))
        self._notify_input_chunk(chunk)

    def get_current_virtual_time(self) -> datetime:
        return self._time

    def get_frame_rate(self) -> float:
        assert not self._closed
        return self._frame_rate

    def get_position(self) -> int:
        assert not self._closed
        return self._position

    def set_position(self, position: int) -> None:
        assert not self._closed
        self._position = position

    def is_transport_rolling(self) -> bool:
        assert not self._closed
        return self._is_transport_rolling

    def set_transport_rolling(self, rolling: bool) -> None:
        assert not self._closed
        self._is_transport_rolling = rolling

    def set_current_playspec(self, playspec: Playspec) -> None:
        assert not self._closed
        self._playspec = playspec

    def close(self) -> None:
        assert not self._closed
        self._closed = True

    def is_closed(self) -> bool:
        return self._closed

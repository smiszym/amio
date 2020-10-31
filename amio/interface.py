from amio.audio_clip import InputAudioChunk
from amio.playspec import Playspec
from typing import Callable


InputChunkCallback = Callable[[InputAudioChunk], None]


class Interface:
    def __init__(self):
        self._input_chunk_callback = None

    @property
    def input_chunk_callback(self) -> InputChunkCallback:
        return self._input_chunk_callback

    @input_chunk_callback.setter
    def input_chunk_callback(self, callback: InputChunkCallback):
        self._input_chunk_callback = callback

    def secs_to_frame(self, seconds: float) -> int:
        return int(self.get_frame_rate() * seconds)

    def frame_to_secs(self, frame: int) -> float:
        return frame / self.get_frame_rate()

    def init(self, client_name: str) -> None:
        raise NotImplementedError

    def get_frame_rate(self) -> float:
        raise NotImplementedError

    def get_position(self) -> int:
        raise NotImplementedError

    def set_position(self, position: int) -> None:
        raise NotImplementedError

    def is_transport_rolling(self) -> bool:
        raise NotImplementedError

    def set_transport_rolling(self, rolling: bool) -> None:
        raise NotImplementedError

    def set_current_playspec(self, playspec: Playspec) -> None:
        raise NotImplementedError

    def close(self) -> None:
        raise NotImplementedError

    def is_closed(self) -> bool:
        raise NotImplementedError

    @property
    def closed(self) -> bool:
        return self.is_closed()

    def _notify_input_chunk(self, data: InputAudioChunk) -> None:
        if self._input_chunk_callback is not None:
            self._input_chunk_callback(data)

from amio.audio_clip import InputAudioChunk
from amio.playspec import Playspec
from typing import Callable


InputChunkCallback = Callable[[InputAudioChunk], None]


class Interface:
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

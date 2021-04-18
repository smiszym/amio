from amio.audio_clip import InputAudioChunk
from amio.playspec import Playspec
from collections import deque, namedtuple
from typing import Callable, Dict, Optional

InputChunkCallback = Callable[[InputAudioChunk], None]
PlayspecChangeCallback = Callable[[bool], None]


def _notify_all(*callbacks):
    def do_notify(was_used):
        for callback in callbacks:
            callback(was_used)

    return do_notify


class PlayspecChange(
    namedtuple("PlayspecChange", "playspec insert_at start_from callback")
):
    pass


class Interface:
    def __init__(self):
        super().__init__()
        self._input_chunk_callback = None
        self._submitted_playspec_callbacks: Dict[int, PlayspecChangeCallback] = {}
        self._pending_playspecs = deque()

    @property
    def input_chunk_callback(self) -> InputChunkCallback:
        return self._input_chunk_callback

    @input_chunk_callback.setter
    def input_chunk_callback(self, callback: InputChunkCallback):
        self._input_chunk_callback = callback

    def schedule_playspec_change(
        self,
        playspec: Playspec,
        insert_at: int,
        start_from: int,
        callback: Optional[PlayspecChangeCallback],
    ):
        if self.is_closed():
            raise ValueError("Operation on a closed AMIO interface")
        playspec_id = self._set_current_playspec(playspec, insert_at, start_from)
        if playspec_id is None:
            self._pending_playspecs.append(
                PlayspecChange(playspec, insert_at, start_from, callback)
            )
        else:
            if callback:
                self._submitted_playspec_callbacks[playspec_id] = callback

    def secs_to_frame(self, seconds: float) -> int:
        return int(self.get_frame_rate() * seconds)

    def frame_to_secs(self, frame: int) -> float:
        return frame / self.get_frame_rate()

    async def init(self, client_name: str) -> None:
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

    def _set_current_playspec(
        self, playspec: Playspec, insert_at: int, start_from: int
    ) -> Optional[int]:
        raise NotImplementedError

    async def close(self) -> None:
        raise NotImplementedError

    def is_closed(self) -> bool:
        raise NotImplementedError

    @property
    def closed(self) -> bool:
        return self.is_closed()

    def _notify_input_chunk(self, data: InputAudioChunk) -> None:
        if self._input_chunk_callback is not None:
            self._input_chunk_callback(data)

    def _retry_setting_playspec_if_needed(self) -> None:
        while self._pending_playspecs:
            playspec = self._pending_playspecs.popleft()
            playspec_id = self._set_current_playspec(
                playspec.playspec, playspec.insert_at, playspec.start_from
            )
            if playspec_id is None:
                self._pending_playspecs.appendleft(playspec)
                # We need to wait for the next opportunity
                break
            else:
                if playspec.callback:
                    self._submitted_playspec_callbacks[playspec_id] = playspec.callback

    def _on_playspec_applied(self, playspec_id: int):
        for i in list(self._submitted_playspec_callbacks.keys()):
            if i < playspec_id:
                # This playspec wasn't actually used and will never be used
                # in the future
                self._submitted_playspec_callbacks[i](False)
                del self._submitted_playspec_callbacks[i]
            elif i == playspec_id:
                # This is the playspec that we were waiting for
                self._submitted_playspec_callbacks[i](True)
                del self._submitted_playspec_callbacks[i]

import asyncio

from amio.audio_clip import ImmutableAudioClip, InputAudioChunk, AudioClip
import amio._native
from amio.interface import Interface
from amio.playspec import Playspec
from datetime import datetime, timezone
from enum import Enum
import logging
import numpy as np
from typing import List, Optional


logger = logging.getLogger("amio")


class PythonQueueProcessingResult(Enum):
    NOTHING = 0
    PLAYSPEC_APPLIED = 1


class NativeInterface(Interface):
    def __init__(self):
        super().__init__()
        self.jack_interface = None
        self.message_task = None
        self._keepalive_clips: List[Optional[ImmutableAudioClip]] = []
        self._pending_logs = ""

    async def init(self, client_name: str) -> None:
        if self.jack_interface is not None:
            raise ValueError(
                "Attempt to initialize an already initialized AMIO interface"
            )
        self.jack_interface = amio._native.create_jack_interface(client_name)
        self.message_task = asyncio.create_task(self._process_messages_and_print_logs())

    async def _process_messages_and_print_logs(self) -> None:
        try:
            while True:
                result = PythonQueueProcessingResult(
                    amio._native.iface_process_messages_on_python_queue(
                        self.jack_interface
                    )
                )
                if result == PythonQueueProcessingResult.PLAYSPEC_APPLIED:
                    playspec_id = amio._native.iface_get_current_playspec_id(
                        self.jack_interface
                    )
                    if playspec_id >= 0:
                        self._on_playspec_applied(playspec_id)
                    self._retry_setting_playspec_if_needed()
                self._collect_and_print_logs()
                while True:
                    input_chunk = self._get_next_input_chunk()
                    if input_chunk is not None:
                        self._notify_input_chunk(input_chunk)
                    if input_chunk is None:
                        break
                await asyncio.sleep(0.1)
        except asyncio.CancelledError:
            pass

    def _collect_and_print_logs(self) -> None:
        # Get any new logs from the IO thread.
        arr = bytearray(4096)
        amio._native.iface_get_logs(self.jack_interface, arr)
        log_content, _, _ = arr.partition(b"\x00")
        self._pending_logs += log_content.decode("utf-8")

        # Print all pending logs up to the last newline.
        # The remaining part (after the last newline) will be printed later.
        lines = self._pending_logs.split("\n")
        self._pending_logs = lines[-1]
        for line in lines[:-1]:
            logger.debug(line)

    def get_frame_rate(self) -> float:
        if self.jack_interface is None:
            raise ValueError("Operation on a closed AMIO interface")
        return amio._native.iface_get_frame_rate(self.jack_interface)

    def get_position(self) -> int:
        if self.jack_interface is None:
            raise ValueError("Operation on a closed AMIO interface")
        return amio._native.iface_get_position(self.jack_interface)

    def set_position(self, position: int) -> None:
        if self.jack_interface is None:
            raise ValueError("Operation on a closed AMIO interface")
        amio._native.iface_set_position(self.jack_interface, position)

    def is_transport_rolling(self) -> bool:
        if self.jack_interface is None:
            raise ValueError("Operation on a closed AMIO interface")
        return amio._native.iface_get_transport_rolling(self.jack_interface) != 0

    def set_transport_rolling(self, rolling: bool) -> None:
        if self.jack_interface is None:
            raise ValueError("Operation on a closed AMIO interface")
        amio._native.iface_set_transport_rolling(
            self.jack_interface, 1 if rolling else 0
        )

    def generate_immutable_clip(self, audio_clip: AudioClip) -> ImmutableAudioClip:
        interface_frame_rate = self.get_frame_rate()
        assert audio_clip.frame_rate == interface_frame_rate
        return ImmutableAudioClip(
            self,
            audio_clip.get_immutable_clip_data(),
            audio_clip.channels,
            interface_frame_rate,
        )

    def _set_current_playspec(
        self, playspec: Playspec, insert_at: int, start_from: int
    ) -> Optional[int]:
        if not amio._native.begin_defining_playspec(
            len(playspec), insert_at, start_from
        ):
            raise RuntimeError("AMIO bug: playspec already being defined")
        self._keepalive_clips = [None for _ in range(len(playspec))]
        for n, entry in enumerate(playspec):
            # Storing in the list to keep this ImmutableAudioClip alive
            if isinstance(entry.clip, ImmutableAudioClip):
                self._keepalive_clips[n] = entry.clip
                entry.clip.use_as_playspec_entry(
                    n,
                    entry.frame_a,
                    entry.frame_b,
                    entry.play_at_frame,
                    entry.repeat_interval,
                    entry.gain_l,
                    entry.gain_r,
                )
            elif isinstance(entry.clip, AudioClip):
                immutable_clip = self.generate_immutable_clip(entry.clip)
                self._keepalive_clips[n] = immutable_clip
                immutable_clip.use_as_playspec_entry(
                    n,
                    entry.frame_a,
                    entry.frame_b,
                    entry.play_at_frame,
                    entry.repeat_interval,
                    entry.gain_l,
                    entry.gain_r,
                )
            else:
                raise ValueError("Wrong audio clip type")
        playspec_id = amio._native.iface_set_playspec(self.jack_interface)
        if playspec_id < 0:
            # Failed to set playspec; most probably the previously set
            # playspec was not yet applied. We should retry later.
            return None
        return playspec_id

    async def close(self) -> None:
        self.message_task.cancel()
        await self.message_task
        amio._native.iface_close(self.jack_interface)

    def is_closed(self) -> bool:
        return self.message_task is None

    def _get_next_input_chunk(self) -> Optional[InputAudioChunk]:
        success = amio._native.iface_begin_reading_input_chunk(self.jack_interface)
        if not success:
            return None
        buf = bytearray(128 * 4)  # 128 float samples
        if amio._native.InputChunk_get_samples(buf) == 0:
            raise AssertionError("AMIO bug: invalid buffer length")
        playspec_id = amio._native.InputChunk_get_playspec_id()
        starting_frame = amio._native.InputChunk_get_starting_frame()
        was_transport_rolling = amio._native.InputChunk_get_was_transport_rolling() != 0
        array = np.frombuffer(buf, dtype=np.float32)
        array = np.reshape(array, (array.shape[0] // 2, 2))
        frame_rate = self.get_frame_rate()
        return InputAudioChunk(
            array,
            frame_rate,
            playspec_id,
            starting_frame,
            was_transport_rolling,
            datetime.now(timezone.utc),
        )

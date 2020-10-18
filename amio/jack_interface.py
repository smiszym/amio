from amio.audio_clip import ImmutableAudioClip, InputAudioChunk, AudioClip
import amio._core
from amio.interface import Interface, InputChunkCallback
from amio.playspec import Playspec
from datetime import datetime
import logging
import numpy as np
import threading
from time import sleep
from typing import List, Optional


logger = logging.getLogger('amio')


class JackInterface(Interface):
    def __init__(self):
        self.jack_interface = None
        self.message_thread = None
        self._keepalive_clips: List[Optional[ImmutableAudioClip]] = []
        self.should_stop = False
        self.should_stop_lock = threading.Lock()
        self._input_chunk_callback = None
        self._pending_logs = ""

    @property
    def input_chunk_callback(self) -> InputChunkCallback:
        return self._input_chunk_callback

    @input_chunk_callback.setter
    def input_chunk_callback(self, callback: InputChunkCallback):
        self._input_chunk_callback = callback

    def _notify_input_chunk(self, data: InputAudioChunk) -> None:
        if self._input_chunk_callback is not None:
            self._input_chunk_callback(data)

    def init(self, client_name: str) -> None:
        self.jack_interface = amio._core.jackio_init(client_name)
        self.message_thread = threading.Thread(
            target=self._process_messages_and_print_logs,
            name='Python message thread')
        self.message_thread.start()

    def _process_messages_and_print_logs(self) -> None:
        should_stop = False
        while not should_stop:
            amio._core.jackio_process_messages_on_python_queue(
                self.jack_interface)
            self._collect_and_print_logs()
            while True:
                input_chunk = self._get_next_input_chunk()
                if input_chunk is not None:
                    self._notify_input_chunk(input_chunk)
                if input_chunk is None:
                    break
            sleep(0.1)
            with self.should_stop_lock:
                should_stop = self.should_stop

    def _collect_and_print_logs(self) -> None:
        # Get any new logs from the IO thread.
        arr = bytearray(4096)
        amio._core.jackio_get_logs(self.jack_interface, arr)
        log_content, _, _ = arr.partition(b'\x00')
        self._pending_logs += log_content.decode("utf-8")

        # Print all pending logs up to the last newline.
        # The remaining part (after the last newline) will be printed later.
        lines = self._pending_logs.split('\n')
        self._pending_logs = lines[-1]
        for line in lines[:-1]:
            logger.debug(line)

    def get_frame_rate(self) -> float:
        return amio._core.jackio_get_frame_rate(self.jack_interface)

    def get_position(self) -> int:
        return amio._core.jackio_get_position(self.jack_interface)

    def set_position(self, position: int) -> None:
        amio._core.jackio_set_position(self.jack_interface, position)

    def is_transport_rolling(self) -> bool:
        return amio._core.jackio_get_transport_rolling(self.jack_interface) != 0

    def set_transport_rolling(self, rolling: bool) -> None:
        amio._core.jackio_set_transport_rolling(
            self.jack_interface,
            1 if rolling else 0)

    def generate_immutable_clip(self,
                                audio_clip: AudioClip) -> ImmutableAudioClip:
        interface_frame_rate = self.get_frame_rate()
        assert audio_clip.frame_rate == interface_frame_rate
        return ImmutableAudioClip(
            self,
            audio_clip.get_immutable_clip_data(),
            audio_clip.channels,
            interface_frame_rate)

    def set_current_playspec(self, playspec: Playspec) -> None:
        size = len(playspec.entries)
        amio._core.Playspec_init(size)
        self._keepalive_clips = [None for i in range(size)]
        for n, entry in enumerate(playspec.entries):
            entry = playspec.entries[n]
            # Storing in the list to keep this ImmutableAudioClip alive
            if isinstance(entry.clip, ImmutableAudioClip):
                self._keepalive_clips[n] = entry.clip
                amio._core.Playspec_setEntry(
                    n, entry.clip.io_owned_clip,
                    entry.frame_a, entry.frame_b,
                    entry.play_at_frame, entry.repeat_interval,
                    entry.gain_l, entry.gain_r)
            elif isinstance(entry.clip, AudioClip):
                immutable_clip = (playspec.jack_interface
                                  .generate_immutable_clip(entry.clip))
                self._keepalive_clips[n] = immutable_clip
                amio._core.Playspec_setEntry(
                    n, immutable_clip.io_owned_clip,
                    entry.frame_a, entry.frame_b,
                    entry.play_at_frame, entry.repeat_interval,
                    entry.gain_l, entry.gain_r)
            else:
                raise ValueError("Wrong audio clip type")
        amio._core.Playspec_setInsertionPoints(
            playspec.insert_at, playspec.start_from)
        amio._core.jackio_set_playspec(playspec.jack_interface.jack_interface)

    def close(self) -> None:
        with self.should_stop_lock:
            self.should_stop = True
        self.message_thread.join()
        amio._core.jackio_close(self.jack_interface)
        self.jack_interface = None

    def is_closed(self) -> bool:
        return self.jack_interface is None

    def _get_next_input_chunk(self) -> Optional[InputAudioChunk]:
        input_chunk = amio._core.jackio_get_input_chunk(self.jack_interface)
        if input_chunk is None:
            return None
        buf = bytearray(128 * 4)  # 128 float samples
        if amio._core.InputChunk_get_samples(input_chunk, buf) == 0:
            raise AssertionError("AMIO bug: invalid buffer length")
        starting_frame = amio._core.InputChunk_get_starting_frame(
            input_chunk)
        was_transport_rolling = (amio._core
            .InputChunk_get_was_transport_rolling(input_chunk) != 0)
        amio._core.InputChunk_del(input_chunk)
        array = np.frombuffer(buf, dtype=np.float32)
        array = np.reshape(array, (array.shape[0] // 2, 2))
        frame_rate = self.get_frame_rate()
        return InputAudioChunk(
            array, frame_rate, starting_frame, was_transport_rolling,
            datetime.now())

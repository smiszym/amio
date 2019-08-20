from amio.audio_clip import ImmutableAudioClip, InputAudioChunk
from amio.interface import Interface
import amio.core
import numpy as np
import threading
from time import sleep


class JackInterface(Interface):
    def __init__(self):
        self.jack_interface = None
        self.message_thread = None
        self.should_stop = False
        self.should_stop_lock = threading.Lock()
        self._input_chunk_callback = None
        self._pending_logs = ""

    @property
    def input_chunk_callback(self):
        return self._input_chunk_callback

    @input_chunk_callback.setter
    def input_chunk_callback(self, callback):
        self._input_chunk_callback = callback

    def _notify_input_chunk(self, data):
        if self._input_chunk_callback is not None:
            self._input_chunk_callback(data)

    def init(self, client_name):
        self.jack_interface = amio.core.jackio_init(client_name)
        self.message_thread = threading.Thread(
            target=self._process_messages_and_print_logs,
            name='Python message thread')
        self.message_thread.start()

    def _process_messages_and_print_logs(self):
        should_stop = False
        while not should_stop:
            amio.core.jackio_process_messages_on_python_queue(
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

    def _collect_and_print_logs(self):
        # Get any new logs from the IO thread.
        arr = bytearray(4096)
        amio.core.jackio_get_logs(self.jack_interface, arr)
        log_content, _, _ = arr.partition(b'\x00')
        self._pending_logs += log_content.decode("utf-8")

        # Print all pending logs up to the last newline.
        # The remaining part (after the last newline) will be printed later.
        lines = self._pending_logs.split('\n')
        self._pending_logs = lines[-1]
        for line in lines[:-1]:
            print(line)

    def get_frame_rate(self):
        return amio.core.jackio_get_frame_rate(self.jack_interface)

    def get_position(self):
        return amio.core.jackio_get_position(self.jack_interface)

    def set_position(self, position):
        amio.core.jackio_set_position(self.jack_interface, position)

    def is_transport_rolling(self):
        return amio.core.jackio_get_transport_rolling(self.jack_interface) != 0

    def set_transport_rolling(self, rolling):
        amio.core.jackio_set_transport_rolling(
            self.jack_interface,
            1 if rolling else 0)

    def generate_immutable_clip(self, audio_clip):
        return ImmutableAudioClip(
            self,
            audio_clip.get_immutable_clip_data(),
            audio_clip.channels,
            audio_clip.frame_rate)

    def set_current_playspec(self, playspec):
        amio.core.jackio_set_playspec(
            self.jack_interface, playspec._immutable_playspec.playspec)

    def close(self):
        with self.should_stop_lock:
            self.should_stop = True
        self.message_thread.join()
        amio.core.jackio_close(self.jack_interface)
        self.jack_interface = None

    def is_closed(self):
        return self.jack_interface is None

    def _get_next_input_chunk(self):
        input_chunk = amio.core.jackio_get_input_chunk(self.jack_interface)
        if input_chunk is not None:
            buf = bytearray(128 * 4)  # 128 float samples
            if amio.core.InputChunk_get_samples(input_chunk, buf) == 0:
                # TODO This shouldn't happen! Raise an error here somehow
                return
            starting_frame = amio.core.InputChunk_get_starting_frame(
                input_chunk)
            was_transport_rolling = (amio.core
                .InputChunk_get_was_transport_rolling(input_chunk) != 0)
            amio.core.InputChunk_del(input_chunk)
            array = np.frombuffer(buf, dtype=np.float32)
            array = np.reshape(array, (array.shape[0] // 2, 2))
            frame_rate = self.get_frame_rate()
            return InputAudioChunk(
                array, frame_rate, starting_frame, was_transport_rolling)

from amio.audio_clip import ImmutableAudioClip, AudioClip
import amio._core
from typing import List, Optional


class ImmutablePlayspec:
    def __init__(self, jack_interface: 'amio.jack_interface.JackInterface',
                 size: int):
        self._jack_interface = jack_interface
        self.playspec = amio._core.Playspec_init(size)
        self._entries: List[Optional[ImmutableAudioClip]] = [
            None for i in range(size)]

    def __del__(self):
        amio._core.Playspec_del(
            self._jack_interface.jack_interface,
            self.playspec)

    def set_entry(self, n: int, clip: AudioClip,
                  frame_a: int, frame_b: int,
                  play_at_frame: int, repeat_interval: int,
                  gain_l: float, gain_r: float) -> None:
        # Storing in the list to keep this ImmutableAudioClip alive
        if isinstance(clip, ImmutableAudioClip):
            self._entries[n] = clip
            amio._core.Playspec_setEntry(
                self.playspec, n, clip.io_owned_clip,
                frame_a, frame_b,
                play_at_frame, repeat_interval,
                gain_l, gain_r)
        elif isinstance(clip, AudioClip):
            immutable_clip = (self._jack_interface
                                .generate_immutable_clip(clip))
            self._entries[n] = immutable_clip
            amio._core.Playspec_setEntry(
                self.playspec, n, immutable_clip.io_owned_clip,
                frame_a, frame_b,
                play_at_frame, repeat_interval,
                gain_l, gain_r)
        else:
            raise ValueError("Wrong audio clip type")

    def set_length(self, length: int) -> None:
        amio._core.Playspec_setLength(self.playspec, length)

    def set_insertion_points(self, insert_at: int, start_from: int) -> None:
        amio._core.Playspec_setInsertionPoints(
            self.playspec, insert_at, start_from)


class Playspec:
    def __init__(self, jack_interface: 'amio.jack_interface.JackInterface',
                 size: int):
        self._immutable_playspec = ImmutablePlayspec(jack_interface, size)

    def set_entry(self, n: int, clip: AudioClip,
                  frame_a: int, frame_b: int,
                  play_at_frame: int, repeat_interval: int,
                  gain_l: float, gain_r: float) -> None:
        self._immutable_playspec.set_entry(
            n, clip,
            frame_a, frame_b,
            play_at_frame, repeat_interval,
            gain_l, gain_r)

    def set_length(self, length: int) -> None:
        self._immutable_playspec.set_length(length)

    def set_insertion_points(self, insert_at: int, start_from: int) -> None:
        self._immutable_playspec.set_insertion_points(insert_at, start_from)

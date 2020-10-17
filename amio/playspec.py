from amio.audio_clip import ImmutableAudioClip, AudioClip
import amio._core
from collections import namedtuple
from typing import List, Optional


class PlayspecEntry(namedtuple(
        'PlayspecEntry',
        'clip frame_a frame_b play_at_frame repeat_interval gain_l gain_r')):
    pass


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

    @classmethod
    def from_playspec(cls, playspec: 'Playspec') -> 'ImmutablePlayspec':
        result = cls(playspec._jack_interface, len(playspec._entries))
        for n, entry in enumerate(playspec._entries):
            entry = playspec._entries[n]
            result.set_entry(
                n, entry.clip, entry.frame_a, entry.frame_b,
                entry.play_at_frame, entry.repeat_interval,
                entry.gain_l, entry.gain_r)
        result.set_length(playspec._length)
        result.set_insertion_points(playspec._insert_at, playspec._start_from)
        return result

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
        self._jack_interface = jack_interface
        self._length = 0
        self._insert_at = 0
        self._start_from = 0
        self._entries: List[Optional[PlayspecEntry]] = [None] * size

    def set_entry(self, n: int, clip: AudioClip,
                  frame_a: int, frame_b: int,
                  play_at_frame: int, repeat_interval: int,
                  gain_l: float, gain_r: float) -> None:
        self._entries[n] = PlayspecEntry(clip, frame_a, frame_b, play_at_frame,
                                         repeat_interval, gain_l, gain_r)

    def set_length(self, length: int) -> None:
        self._length = length

    def set_insertion_points(self, insert_at: int, start_from: int) -> None:
        self._insert_at = insert_at
        self._start_from = start_from

from amio.audio_clip import ImmutableAudioClip, AudioClip
import amio._core
from collections import namedtuple
from typing import List, Optional


class PlayspecEntry(namedtuple(
        'PlayspecEntry',
        'clip frame_a frame_b play_at_frame repeat_interval gain_l gain_r')):
    pass


class Playspec:
    def __init__(self, jack_interface: 'amio.jack_interface.JackInterface',
                 size: int):
        self.jack_interface = jack_interface
        self.insert_at = 0
        self.start_from = 0
        self.entries: List[Optional[PlayspecEntry]] = [None] * size

    def set_entry(self, n: int, clip: AudioClip,
                  frame_a: int, frame_b: int,
                  play_at_frame: int, repeat_interval: int,
                  gain_l: float, gain_r: float) -> None:
        self.entries[n] = PlayspecEntry(clip, frame_a, frame_b, play_at_frame,
                                        repeat_interval, gain_l, gain_r)

    def set_insertion_points(self, insert_at: int, start_from: int) -> None:
        self.insert_at = insert_at
        self.start_from = start_from

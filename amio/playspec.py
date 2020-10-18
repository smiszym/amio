from amio.audio_clip import ImmutableAudioClip, AudioClip
import amio._core
from collections import namedtuple
from typing import List, Optional


class PlayspecEntry(namedtuple(
        'PlayspecEntry',
        'clip frame_a frame_b play_at_frame repeat_interval gain_l gain_r')):
    pass


def set_playspec_as_current(playspec: 'Playspec'):
    size = len(playspec.entries)
    amio._core.Playspec_init(size)
    keepalive_clips: List[Optional[ImmutableAudioClip]] = [
        None for i in range(size)]
    for n, entry in enumerate(playspec.entries):
        entry = playspec.entries[n]
        # Storing in the list to keep this ImmutableAudioClip alive
        if isinstance(entry.clip, ImmutableAudioClip):
            keepalive_clips[n] = entry.clip
            amio._core.Playspec_setEntry(
                n, entry.clip.io_owned_clip,
                entry.frame_a, entry.frame_b,
                entry.play_at_frame, entry.repeat_interval,
                entry.gain_l, entry.gain_r)
        elif isinstance(entry.clip, AudioClip):
            immutable_clip = (playspec.jack_interface
                              .generate_immutable_clip(entry.clip))
            keepalive_clips[n] = immutable_clip
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
    return keepalive_clips


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

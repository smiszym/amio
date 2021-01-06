from amio.audio_clip import ImmutableAudioClip
import amio._native
from collections import namedtuple
from typing import List


class PlayspecEntry(
    namedtuple(
        "PlayspecEntry",
        "clip frame_a frame_b play_at_frame repeat_interval gain_l gain_r",
    )
):
    @property
    def start(self):
        return self.play_at_frame

    @start.setter
    def start(self, new_start):
        shorten_by = new_start - self.start
        self.play_at_frame += shorten_by
        self.frame_a += shorten_by
        if self.frame_a > self.frame_b:
            self.frame_a = self.frame_b

    @property
    def end(self):
        return self.play_at_frame + len(self)

    @end.setter
    def end(self, new_end):
        shorten_by = self.end - new_end
        self.frame_b -= shorten_by
        if self.frame_b < self.frame_a:
            self.frame_b = self.frame_a

    def __len__(self):
        return self.frame_b - self.frame_a


Playspec = List[PlayspecEntry]

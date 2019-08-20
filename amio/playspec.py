import amio.core
from amio.audio_clip import ImmutableAudioClip, AudioClip


class ImmutablePlayspec:
    def __init__(self, jack_interface, size):
        self._jack_interface = jack_interface
        self.playspec = amio.core.Playspec_init(size)
        self._entries = [None for i in range(size)]

    def __del__(self):
        amio.core.Playspec_del(
            self._jack_interface.jack_interface,
            self.playspec)

    def set_entry(self, n, clip,
                  frame_a, frame_b,
                  play_at_frame, repeat_interval,
                  gain_l, gain_r):
        # Storing in the list to keep this ImmutableAudioClip alive
        if isinstance(clip, ImmutableAudioClip):
            self._entries[n] = clip
        elif isinstance(clip, AudioClip):
            self._entries[n] = (self._jack_interface
                                .generate_immutable_clip(clip))
        else:
            raise ValueError("Wrong audio clip type")
        amio.core.Playspec_setEntry(
            self.playspec, n, self._entries[n].io_owned_clip,
            frame_a, frame_b,
            play_at_frame, repeat_interval,
            gain_l, gain_r)

    def set_length(self, length):
        amio.core.Playspec_setLength(self.playspec, length)

    def set_insertion_points(self, insert_at, start_from):
        amio.core.Playspec_setInsertionPoints(
            self.playspec, insert_at, start_from)


class Playspec:
    def __init__(self, jack_interface, size):
        self._immutable_playspec = ImmutablePlayspec(jack_interface, size)

    def set_entry(self, n, clip,
                  frame_a, frame_b,
                  play_at_frame, repeat_interval,
                  gain_l, gain_r):
        self._immutable_playspec.set_entry(
            n, clip,
            frame_a, frame_b,
            play_at_frame, repeat_interval,
            gain_l, gain_r)

    def set_length(self, length):
        self._immutable_playspec.set_length(length)

    def set_insertion_points(self, insert_at, start_from):
        self._immutable_playspec.set_insertion_points(insert_at, start_from)

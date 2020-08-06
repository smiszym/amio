from __future__ import annotations
from amio.fader import factor_to_dB
import matplotlib.pyplot as plt
import numpy as np
import amio.core
import os
import soundfile as sf
import struct
from subprocess import Popen
from tempfile import NamedTemporaryFile


class ImmutableAudioClip:
    """
    For internal AMIO use only.

    An immutable clip of 16-bit native-endian channel-interleaved audio data.
    The length is determined by the length of the data-supplying bytes object.
    Sample rate is expected to be equal to the audio output sample rate.
    Objects of this class exist primarily to automate garbage collection.
    When ImmutableAudioClip is destroyed, the I/O thread is informed
    and is then free to schedule object destruction when no longer needs it.
    """
    def __init__(self, jack_client,
                 data: bytes, channels: int, frame_rate: float):
        if not isinstance(channels, int) or channels < 1:
            raise TypeError(
                "Invalid number of channels (must be positive integer)")
        self.jack_client = jack_client
        self.io_owned_clip = amio.core.AudioClip_init(
            data, channels, frame_rate)

    def __del__(self):
        amio.core.AudioClip_del(
            self.jack_client.jack_interface,
            self.io_owned_clip)
        self.io_owned_clip = None


class AudioClip:
    """
    A class aggregating NumPy array representing audio data and its sample rate.
    The array is a NumPy array of shape (length_in_frames, channel_count).
    There is one column in the array for every channel.
    The array datatype is float and the sample range is [-1.0, 1.0].
    """
    def __init__(self, array, frame_rate: float):
        assert isinstance(array, np.ndarray)
        assert np.issubdtype(array.dtype, np.floating)
        if len(array.shape) == 1:
            # Mono audio
            self._array = np.reshape(array, (array.shape[0], 1))
        elif len(array.shape) == 2:
            # Multichannel audio (mono, stereo, or more)
            self._array = array
        else:
            raise ValueError("Incorrect array shape (must be 1D or 2D)")
        self.frame_rate = frame_rate
        self._immutable_clip_data = None

    def __len__(self):
        return self._array.shape[0]

    def __str__(self):
        return f"{(len(self) / self.frame_rate):.1f} s," \
               f" {self.calculate_rms_power():.1f} dB AudioClip"

    @property
    def channels(self):
        return self._array.shape[1]

    @property
    def writeable(self):
        return self._array.flags.writeable

    @writeable.setter
    def writeable(self, value):
        """
        Make AudioClip expect or not changes to the underlying array.
        WARNING: You must not set flags.writeable of the underlying array
        directly, because AudioClip caches some properties of the data
        if it thinks it's not writeable. So if you want to change the data,
        always set AudioClip.writeable = True if it's not yet.
        The default is True.
        :param value: Whether the underlying NumPy array is writeable.
        """
        self._array.flags.writeable = value
        if value:
            self._immutable_clip_data = None  # invalidate cached value

    @property
    def array(self):
        """
        Return the underlying NumPy array. Note: the array object may be changed
        to a new one on certain operations, like resize.
        WARNING: You must not change the contents of the array if
        AudioClip.writeable is False! Set it to True first. This is because
        AudioClip caches some properties of the data if it's not writeable.
        """
        return self._array

    @property
    def memory_usage_mb(self):
        return self._array.nbytes / 1024 / 1024

    def calculate_rms_power(self):
        """
        Calculates total signal power, over the whole signal duration.
        :return: Total signal power (dB)
        """
        return factor_to_dB(np.sqrt(np.mean(self._array ** 2)))

    def get_immutable_clip_data(self):
        if self._immutable_clip_data is not None:
            return self._immutable_clip_data
        calculated = ((self._array * 32767)
                      .clip(-32767, 32767).astype(np.int16).tobytes())
        if not self._array.flags.writeable:
            self._immutable_clip_data = calculated  # cache the result
        return calculated

    def channel(self, channel_number):
        return AudioClip(self._array[:,channel_number], self.frame_rate)

    def overwrite(self, patch_clip, position, clip_a=0, clip_b=-1,
                  extend_to_fit=False):
        """
        Overwrite part of this audio clip with patch_clip. By default
        the whole patch_clip is put onto this audio clip, but this can be
        changed by specifying boundaries with clip_a and clip_b.
        This method operates in-place and doesn't return anything.
        :param patch_clip: Audio clip to put to overwrite existing content.
        :param position: Position in frames at which to put the clip.
        :param clip_a: Starting frame of the patch clip.
        :param clip_b: End frame of the patch clip.
        :param extend_to_fit: If True, the clip will be resized if too small
        to contain the clip being inserted.
        """
        assert patch_clip.channels == self.channels
        if clip_b == -1:
            clip_b = len(patch_clip)
        if position < 0:
            clip_a += (-position)
            position = 0
        inserted_length = clip_b - clip_a
        if inserted_length < 0:
            return
        if position > len(self) and not extend_to_fit:
            return
        if position + inserted_length > len(self):
            if extend_to_fit:
                self.resize(position + inserted_length)
            else:
                to_cut = position + inserted_length - len(self)
                inserted_length -= to_cut
                clip_b -= to_cut
        self._array[position:position+inserted_length,:] = (
            patch_clip._array[clip_a:clip_b,:])

    def resampled_if_needed(self,
                            required_frame_rate: float,
                            epsilon: float=0.1):
        if self.channels != 1:
            raise ValueError("Resampling is only supported for mono clips")
        if abs(self.frame_rate - required_frame_rate) <= epsilon:
            return self
        length_seconds = len(self) / self.frame_rate
        new_array = (np.transpose(np.array([np.interp(
            np.linspace(0, length_seconds, int(len(self)
                                               * required_frame_rate
                                               / self.frame_rate)),
            np.linspace(0, length_seconds, len(self)),
            self._array[:, i]) for i in range(self.channels)]))
            / 32768)
        return AudioClip(new_array, required_frame_rate)

    @staticmethod
    def from_soundfile(filename):
        if os.path.getsize(filename) == 0:
            return
        arr, frame_rate = sf.read(filename)
        return AudioClip(arr, frame_rate)

    def to_soundfile(self, filename):
        sf.write(filename, self._array, int(self.frame_rate))

    def create_metering_data(self, metering_fps=24):
        metering_window = self.frame_rate / metering_fps
        num_fragments = int(self._array.shape[0] // metering_window)
        if num_fragments < 1:
            num_fragments = 1
        rms = [factor_to_dB(np.sqrt(np.mean(fragment ** 2)))
               for fragment in np.array_split(self._array, num_fragments)]
        return num_fragments, rms

    def resize(self, new_length):
        """
        Resize the clip in-place (although the underlying NumPy array object
        will be changed to a new object). Will lose data if the new length
        is less than the current length.
        """
        assert self.writeable
        current_length = len(self)
        if new_length == current_length:
            pass
        elif new_length < current_length:
            self._array = self._array[:new_length]
        else:
            self._array = np.concatenate((
                self._array,
                np.zeros((new_length - current_length, self.channels))))

    def open_in_audacity(self):
        """
        A utility method that creates a temporary file, writes the audio
        data to it, and opens it in Audacity. After Audacity process is
        closed, the file is deleted.
        """
        with NamedTemporaryFile(suffix='.wav') as f:
            self.to_soundfile(f)
            f.flush()
            pipe = Popen(["audacity", f.name])
            pipe.wait()

    def plot(self):
        plt.figure(figsize=(len(self) / self.frame_rate, 10))
        plt.gca().set_ylim([-1, 1])
        plt.plot(self._array)
        plt.show()

    @staticmethod
    def zeros(length, channels, frame_rate):
        return AudioClip(np.zeros((length, channels), np.float32), frame_rate)

    @staticmethod
    def sine(frequency, amplitude, length, channels, frame_rate):
        length_seconds = length / frame_rate
        periods = length_seconds * frequency
        sine = amplitude * np.sin(np.linspace(0, 2 * np.pi * periods, length))
        sine = np.reshape(sine, (length, 1))
        return AudioClip(np.hstack(channels * (sine, )), frame_rate)

    @staticmethod
    def from_au_file(filename):
        with open(filename, 'rb') as file:
            data = file.read()
            magic = data[0:4]
            if magic == b".snd":
                endianness = '>'
            elif magic == b"dns.":
                # Audacity incorrectly saves its blockfiles in little-endian
                # (at least on little-endian processors)
                endianness = '<'
            else:
                raise ValueError("Invalid AU file")
            header_length, data_size, encoding, frame_rate, channels = \
                struct.unpack(endianness + 'LLLLL', data[4:24])
            if encoding != 6:
                raise ValueError("The only supported encoding is 6 "
                                 "(32-bit IEEE floating point)")
            if channels != 1:
                raise ValueError("Only mono AU files are supported")

            dt = np.dtype(np.float32)
            dt = dt.newbyteorder(endianness)
            array = np.frombuffer(data[header_length:], dtype=dt)
            return AudioClip(array, frame_rate)

    @staticmethod
    def stereo_clip_from_mono_clips(left: AudioClip, right: AudioClip):
        if left.frame_rate != right.frame_rate:
            raise ValueError("Sample rates must match")
        if left.channels != 1 or right.channels != 1:
            raise ValueError("Both clips must be mono")
        return AudioClip(
            np.hstack((left._array, right._array)), left.frame_rate)

    @staticmethod
    def concatenate(sequence_of_clips):
        clips = list(sequence_of_clips)
        if len(clips) < 1:
            return
        if not all(clip.channels == clips[0].channels for clip in clips):
            raise ValueError("All clips must have the same channel number")
        if not all(clip.frame_rate == clips[0].frame_rate for clip in clips):
            raise ValueError("All clips must have the same sample rate")
        return AudioClip(
            np.vstack([clip._array for clip in clips]),
            clips[0].frame_rate)


class InputAudioChunk(AudioClip):
    def __init__(
            self, array, frame_rate: float,
            starting_frame, was_transport_rolling,
            wall_time):
        super(InputAudioChunk, self).__init__(array, frame_rate)
        self.starting_frame = starting_frame
        self.was_transport_rolling = was_transport_rolling
        self.wall_time = wall_time

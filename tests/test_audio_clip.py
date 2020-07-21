from amio import AudioClip
import numpy as np


def test_mono_audio_clip_basic_properties_1():
    clip = AudioClip(np.zeros((100, 1), np.float32), 48000)
    assert clip.frame_rate == 48000
    assert len(clip) == 100
    assert clip.channels == 1


def test_mono_audio_clip_basic_properties_2():
    clip = AudioClip(np.zeros(100, np.float32), 48000)
    assert clip.frame_rate == 48000
    assert len(clip) == 100
    assert clip.channels == 1


def test_stereo_audio_clip_basic_properties():
    clip = AudioClip(np.zeros((100, 2), np.float32), 48000)
    assert clip.frame_rate == 48000
    assert len(clip) == 100
    assert clip.channels == 2


def test_mono_audio_clip_concatenate():
    clip1 = AudioClip(np.zeros(100, np.float32), 48000)
    clip2 = AudioClip(np.zeros(300, np.float32), 48000)
    result = AudioClip.concatenate((clip1, clip2))
    assert result.frame_rate == 48000
    assert len(result) == 400
    assert result.channels == 1


def test_stereo_audio_clip_concatenate():
    clip1 = AudioClip(np.zeros((100, 2), np.float32), 48000)
    clip2 = AudioClip(np.zeros((300, 2), np.float32), 48000)
    result = AudioClip.concatenate((clip1, clip2))
    assert result.frame_rate == 48000
    assert len(result) == 400
    assert result.channels == 2


def test_resize_downwards():
    clip = AudioClip(np.zeros((100, 2), np.float32), 48000)
    clip.resize(50)
    assert clip.channels == 2
    assert len(clip) == 50


def test_resize_upwards():
    clip = AudioClip(np.zeros((100, 2), np.float32), 48000)
    clip.resize(150)
    assert clip.channels == 2
    assert len(clip) == 150


def test_sine_1():
    # TODO Enhance this test by adding checks for sine properties
    clip = AudioClip.sine(440, 1.0, 48000, 1, 48000)
    assert clip.frame_rate == 48000
    assert clip.channels == 1
    assert len(clip) == 48000


def test_sine_2():
    # TODO Enhance this test by adding checks for sine properties
    clip = AudioClip.sine(440, 1.0, 48000, 2, 48000)
    assert clip.frame_rate == 48000
    assert clip.channels == 2
    assert len(clip) == 48000

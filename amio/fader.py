import math
import sys


# amplitude factor | power factor | power gain
#               1x |      1x      |  0 dB
#            1.41x |      2x      | +3 dB
#               2x |      4x      | +6 dB
#              10x |     100x     | +20 dB


def factor_to_dB(factor):
    """
    Converts amplitude factor to power gain im decibels.
    :param factor: Amplitude factor between two signals
    :return: Power gain between two signals (dB)
    """
    return (20.0 * math.log10(factor)
            if factor > sys.float_info.min
            else float('-inf'))


def dB_to_factor(dB):
    return 10 ** (dB / 20.0)


class Fader:
    def __init__(self, vol_dB=0.0, pan=0.0):
        self._vol_factor = dB_to_factor(vol_dB)
        self._pan = pan

    @property
    def vol_dB(self):
        return factor_to_dB(self._vol_factor)

    @vol_dB.setter
    def vol_dB(self, value_dB):
        self._vol_factor = dB_to_factor(value_dB)

    @property
    def vol_factor(self):
        return self._vol_factor

    @vol_factor.setter
    def vol_factor(self, value_factor):
        self._vol_factor = value_factor

    @property
    def pan(self):
        return self._pan

    @pan.setter
    def pan(self, value):
        assert -1 <= value <= 1
        self._pan = value

    @property
    def left_gain_factor(self):
        return self._vol_factor * (1.0 - self._pan)

    @property
    def right_gain_factor(self):
        return self._vol_factor * (1.0 + self._pan)

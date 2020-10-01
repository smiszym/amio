from amio.interface import Interface


class DummyInterface(Interface):
    """
    Dummy interface that doesn't interact with amio._core at all, thus
    requiring no native code. It doesn't have any functionality, but it's
    capable of frame-to-second conversion, so is suitable for use in tests.
    """
    def __init__(self, frame_rate: float):
        self._frame_rate = frame_rate

    def get_frame_rate(self) -> float:
        return self._frame_rate

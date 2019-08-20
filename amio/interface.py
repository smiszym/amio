class Interface:
    def secs_to_frame(self, seconds):
        return int(self.get_frame_rate() * seconds)

    def frame_to_secs(self, frame):
        return frame / self.get_frame_rate()

    def init(self, client_name):
        raise NotImplementedError

    def get_frame_rate(self):
        raise NotImplementedError

    def get_position(self):
        raise NotImplementedError

    def set_position(self, position):
        raise NotImplementedError

    def is_transport_rolling(self):
        raise NotImplementedError

    def set_transport_rolling(self, rolling):
        raise NotImplementedError

    def set_current_playspec(self, playspec):
        raise NotImplementedError

    def close(self):
        raise NotImplementedError

    def is_closed(self):
        raise NotImplementedError

    @property
    def closed(self):
        return self.is_closed()

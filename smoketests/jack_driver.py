import asyncio

from amio import (
    AudioClip,
    create_io_interface,
    PlayspecEntry,
)


async def main():
    interface = create_io_interface("jack")
    await interface.init("amio-tests")
    interface.set_transport_rolling(True)
    frame_rate = interface.get_frame_rate()

    for i in range(30):
        print(f"Playspec {i + 1} out of 30...")
        clips = [AudioClip.zeros(100000, 2, frame_rate) for _ in range(30)]
        playspec = [
            PlayspecEntry(
                clip=clip,
                frame_a=0,
                frame_b=100000,
                play_at_frame=0,
                repeat_interval=0,
                gain_l=1.0,
                gain_r=1.0,
            )
            for clip in clips
        ]

        interface.schedule_playspec_change(playspec, 0, 0, None)
        await asyncio.sleep(0.3)

    interface.set_transport_rolling(False)
    await interface.close()


if __name__ == "__main__":
    asyncio.run(main())

import asyncio
from time import sleep
from state_machine import StateMachine
from bleak.exc import BleakDeviceNotFoundError, BleakError, BleakGATTProtocolError
from ble_interface import BleInterface
import threading
from app import app
from typing import cast


async def poll_ble_server(interface: BleInterface) -> None:
    while True:
        await interface.poll_ble()
        await asyncio.sleep(3)


def poll_ble_thread(interface: BleInterface, main_loop: asyncio.AbstractEventLoop, stop_event: asyncio.Event, exc_holder: list[Exception]) -> None:
    try:
        asyncio.run_coroutine_threadsafe(poll_ble_server(interface), main_loop).result()
    except (BleakGATTProtocolError, BleakDeviceNotFoundError, BleakError, Exception) as e:
        exc_holder.append(e)
        try:
            _ = main_loop.call_soon_threadsafe(stop_event.set)
        except RuntimeError:
            pass


def state_machine_thread(interface: BleInterface, main_loop: asyncio.AbstractEventLoop, stop_event: asyncio.Event, exc_holder: list[Exception]) -> None:
    async def run() -> None:
        machine = StateMachine(interface)
        await machine.start()
    try:
        asyncio.run_coroutine_threadsafe(run(), main_loop).result()
    except (BleakGATTProtocolError, BleakDeviceNotFoundError, BleakError, Exception) as e:
        exc_holder.append(e)
        try:
            _ = main_loop.call_soon_threadsafe(stop_event.set)
        except RuntimeError:
            pass


async def main(retry: list[int]) -> None:
    interface = await BleInterface().create()
    print("Connected")
    retry[0] = 0

    main_loop = asyncio.get_event_loop()
    stop_event = asyncio.Event()
    exc_holder: list[Exception] = []

    threading.Thread(target=poll_ble_thread, args=(interface, main_loop, stop_event, exc_holder), daemon=True).start()
    threading.Thread(target=state_machine_thread, args=(interface, main_loop, stop_event, exc_holder), daemon=True).start()

    _ = await stop_event.wait()
    raise exc_holder[0]


if __name__ == "__main__":
    threading.Thread(target=app.run, kwargs={"host": "0.0.0.0"}, daemon=True).start()
    
    retry: list[int] = [0]

    while True:
        try:
            asyncio.run(main(retry))
        except (BleakGATTProtocolError, BleakDeviceNotFoundError, BleakError) as e:
            print(e)

            wait_time = min(cast(int, 2 ** retry[0]), 64);
            print(f"Reconnecting in {wait_time}s...")
            sleep(wait_time)
            retry[0] = retry[0] + 1
        except KeyboardInterrupt:
            print("\nExiting...")
            break
        except Exception as e:
            print(e)
            print("Unhandled error, failed to restart.")
            break
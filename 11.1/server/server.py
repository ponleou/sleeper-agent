import asyncio
from state_machine import StateMachine
from bleak.exc import BleakDeviceNotFoundError, BleakError, BleakGATTProtocolError
from ble_interface import BleInterface
import threading
from app import app


async def poll_ble_server(interface: BleInterface):
    while True:
        print("POLL BLE SERVER")
        await interface.poll_ble()
        await asyncio.sleep(3)


def poll_ble_server_thread(interface: BleInterface, loop: asyncio.AbstractEventLoop):
    asyncio.run_coroutine_threadsafe(poll_ble_server(interface), loop).result()

async def main() -> None:
    interface = await BleInterface().create()
    print("connected")

    loop = asyncio.get_event_loop()
    _ = threading.Thread(target=poll_ble_server_thread, args=(interface, loop), daemon=True).start()
    _ = threading.Thread(target=app.run, kwargs={"host": "0.0.0.0"}, daemon=True).start()

    machine = StateMachine(interface)
    await machine.start()


if __name__ == "__main__":
    while True:
        try:
            asyncio.run(main())

        # these are expected errors when disconnected from BLE, just keep trying to reconnect
        except BleakDeviceNotFoundError as e:
            print(e)
            print("Reconnecting...")
        except (BleakError, BleakGATTProtocolError) as e:
            print(e)
            print("Reconnecting...")

        # close server
        except KeyboardInterrupt:
            print("\nExiting...")
            break

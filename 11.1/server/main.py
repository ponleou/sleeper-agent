from datetime import datetime
import asyncio
from bleak import BleakScanner, BleakClient
from bleak.exc import BleakDeviceNotFoundError, BleakError, BleakGATTProtocolError

from models import engine, SessionRecord, NotificationData, SessionMeta
from sqlalchemy.orm import Session
from sqlalchemy import select

# BLUETOOTH_ADDRESS = "94:B5:55:C0:60:32"
BLE_SERVICE_UUID = "00000100-addd-43a2-b9cc-6c8adc8a7761"
BLE_SESSION_CHAR_UUID = "00000101-addd-43a2-b9cc-6c8adc8a7761"
BLE_ENQUEUE_CHAR_UUID = "00000110-addd-43a2-b9cc-6c8adc8a7761"
BLE_START_CHAR_UUID = "00000120-addd-43a2-b9cc-6c8adc8a7761"
BLE_STOP_CHAR_UUID = "00000130-addd-43a2-b9cc-6c8adc8a7761"
BLE_ALERT_CHAR_UUID = "00000140-addd-43a2-b9cc-6c8adc8a7761"

class BleInterface():
    _client: BleakClient | None

    def __init__(self):
        self._client = None

    @classmethod
    async def create(cls) -> 'BleInterface':
        self = cls.__new__(cls)
        self._client = await self._connect_ble_client()
        await self._client.connect()
        return self

    async def _connect_ble_client(self) -> BleakClient:
        device = await BleakScanner.find_device_by_filter(
            lambda dev, ad: BLE_SERVICE_UUID in ad.service_uuids
        )

        if device == None:
            print("ERROR: Cannot connect to BLE device")
            raise BleakError("Connection lost")

        return BleakClient(device)

    async def get_identity(self) -> str:
        if not self._client:
            print("ERROR: Client is not created")
            raise BleakError("Connection lost")

        if not self._client.is_connected:
            print("WARN: Read/write attempt with BLE device failed")
            return " "

        session_id_bytes = await self._client.read_gatt_char(BLE_SESSION_CHAR_UUID)
        session_id = session_id_bytes.decode("utf-8")
        
        return session_id

    async def is_registered(self, session_id: str) -> bool:
        with Session(engine) as session:
            exists = session.execute(select(SessionMeta).where(SessionMeta.session_id == session_id)).scalar_one_or_none()
            if not exists:
                return False
        
        return True
    
    async def dequeue(self, session_id: str) -> None:
        if not self._client:
            print("ERROR: Client is not created")
            raise BleakDeviceNotFoundError(BLE_SERVICE_UUID)

        if not self._client.is_connected:
            print("WARN: Read/write attempt with BLE device failed")
            raise BleakError("Connection lost")

        data = await self._client.read_gatt_char(BLE_ENQUEUE_CHAR_UUID);
        value = data.decode("utf-8")
        if (value == " "):
            return

        await self._client.write_gatt_char(BLE_ENQUEUE_CHAR_UUID, b" ")
        
        print("ID:", session_id)
        print("Received:", value) 

        parts = value.split("|")
        appname = parts[0]
        title = parts[1]
        text = parts[2]
        dt = datetime.fromtimestamp(int(parts[3]) / 1000)

        with Session(engine) as session:
            exists = session.execute(
                select(SessionRecord).where(SessionRecord.session_id == session_id)
            ).scalar_one_or_none()
            if not exists:
                session.add(SessionRecord(session_id=session_id))
            session.add(NotificationData(
                session_id=session_id,
                appname=appname,
                title=title,
                text=text,
                datetime=dt,
            ))
            session.commit()

async def main() -> None:
    interface = await BleInterface().create()

    print("connected")

    while (True):
        id = await interface.get_identity()
        if (id == " "):
            continue

        print(id)

        await interface.dequeue(id)
        await asyncio.sleep(0.2)


if __name__ == "__main__":
    while True:
        try:
            asyncio.run(main())
        except BleakDeviceNotFoundError:
            print("Device not found, retrying")
        except (BleakError, BleakGATTProtocolError):
            print("Disconnected, reconnecting...")
        except KeyboardInterrupt:
            print("\nExiting...")
            break
  
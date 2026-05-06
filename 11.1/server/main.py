from datetime import datetime
import asyncio
from bleak import BleakScanner, BleakClient
from bleak.backends.characteristic import BleakGATTCharacteristic

from models import engine, SessionRecord, NotificationData, SessionMeta
from sqlalchemy.orm import Session
from sqlalchemy import select

BLUETOOTH_ADDRESS = "94:B5:55:C0:60:32"
BLE_SERVICE_UUID = "00000100-addd-43a2-b9cc-6c8adc8a7761"
BLE_SESSION_CHAR_UUID = "00000101-addd-43a2-b9cc-6c8adc8a7761"
BLE_ENQUEUE_CHAR_UUID = "00000110-addd-43a2-b9cc-6c8adc8a7761"
BLE_START_CHAR_UUID = "00000120-addd-43a2-b9cc-6c8adc8a7761"
BLE_STOP_CHAR_UUID = "00000130-addd-43a2-b9cc-6c8adc8a7761"
BLE_ALERT_CHAR_UUID = "00000140-addd-43a2-b9cc-6c8adc8a7761"

class BleInterface():
    _client: BleakClient | None
    _ignore_repeat_notify: bool

    def __init__(self):
        self._client = None
        self._ignore_repeat_notify = False

    @classmethod
    async def create(cls) -> 'BleInterface':
        self = cls.__new__(cls)
        self._client = await self._connect_ble_client()
        await self._client.connect()
        self._ignore_repeat_notify = False
        return self

    async def _connect_ble_client(self) -> BleakClient:
        device = await BleakScanner.find_device_by_filter(
            lambda dev, ad: dev.address.upper() == BLUETOOTH_ADDRESS.upper() and BLE_SERVICE_UUID in ad.service_uuids
        )

        if device == None:
            print("ERROR: Cannot connect to BLE device")
            exit(1)

        return BleakClient(device)

    async def _on_notify_dequeue_char(self, _: BleakGATTCharacteristic, data: bytearray) -> None:
        if not self._client:
            print("ERROR: Client is not created")
            exit(1)

        # if writes are missing, it might be this at fault
        # bluez can repeat notify, not sure about other interfaces
        if self._ignore_repeat_notify:
            self._ignore_repeat_notify = False
            return

        if not self._client.is_connected:
            print("WARN: Read/write attempt with BLE device failed")
            return

        value = data.decode("utf-8")
        self._ignore_repeat_notify = True
        await self._client.write_gatt_char(BLE_ENQUEUE_CHAR_UUID, b"")
        
        session_id_bytes = await self._client.read_gatt_char(BLE_SESSION_CHAR_UUID)
        session_id = session_id_bytes.decode("utf-8").strip()
        
        if not session_id:
            return
        
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
    
    async def start_notify(self) -> None:
        if not self._client:
            print("ERROR: Client is not created")
            exit(1)

        await self._client.start_notify(BLE_ENQUEUE_CHAR_UUID, self._on_notify_dequeue_char)

async def main() -> None:
    interface = await BleInterface().create()

    print("connected")
    await interface.start_notify()

    _ = await asyncio.Event().wait()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nExiting...")
from bleak import BleakScanner, BleakClient
from bleak.exc import BleakDeviceNotFoundError, BleakError
from models import PriorityData, engine, NotificationData, SessionMeta
from sqlalchemy.orm import Session
from sqlalchemy import select
from timezonefinder import TimezoneFinder
from datetime import datetime

BLE_SERVICE_UUID = "00000100-addd-43a2-b9cc-6c8adc8a7761"
BLE_SESSION_CHAR_UUID = "00000101-addd-43a2-b9cc-6c8adc8a7761"
BLE_ENQUEUE_CHAR_UUID = "00000110-addd-43a2-b9cc-6c8adc8a7761"
BLE_LOCK_CHAR_UUID = "00000120-addd-43a2-b9cc-6c8adc8a7761"
BLE_PRIORITY_CHAR_UUID = "00000130-addd-43a2-b9cc-6c8adc8a7761"
BLE_ALERT_CHAR_UUID = "00000140-addd-43a2-b9cc-6c8adc8a7761"
BLE_WEBLINK_CHAR_UUID = "00000150-addd-43a2-b9cc-6c8adc8a7761"
BLE_METADATA_CHAR_UUID = "00000160-addd-43a2-b9cc-6c8adc8a7761"
BLE_SERVERPOLL_CHAR_UUID = "00000170-addd-43a2-b9cc-6c8adc8a7761"
BLE_STATUS_CHAR_UUID = "00000180-addd-43a2-b9cc-6c8adc8a7761"


class BleInterface:
    _client: BleakClient | None

    def __init__(self):
        self._client = None

    @classmethod
    async def create(cls) -> BleInterface:
        self = cls.__new__(cls)
        self._client = await self._connect_ble_client()
        await self._client.connect()
        return self

    async def poll_ble(self) -> None:
        if not self._client:
            print("ERROR: Client is not created")
            raise BleakDeviceNotFoundError(BLE_SERVICE_UUID)

        if not self._client.is_connected:
            print("WARN: Read/write attempt with BLE device failed")
            raise BleakError("Connection lost")

        await self._client.write_gatt_char(
            BLE_SERVERPOLL_CHAR_UUID, (0).to_bytes(4, byteorder="little"))

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

    async def store_session_metadata(self, session_id: str) -> tuple[str, str, str]:
        if not self._client:
            print("ERROR: Client is not created")
            raise BleakDeviceNotFoundError(BLE_SERVICE_UUID)

        if not self._client.is_connected:
            print("WARN: Read/write attempt with BLE device failed")
            raise BleakError("Connection lost")

        data = await self._client.read_gatt_char(BLE_METADATA_CHAR_UUID)
        value = data.decode("utf-8")
        print(value)
        if value == " ":
            return ("", "", "")

        parts = value.split("|")

        # lat long can be "unknown", and is handled
        lat = parts[0]
        long = parts[1]

        # timezone should always be available
        timezone = parts[2]

        # ip can be "unknown" like when device isnt connected to wifi
        # always update entry regardless
        local_ip = parts[3]

        tf = TimezoneFinder()
        with Session(engine) as session:
            session_meta = session.execute(
                select(SessionMeta).where(SessionMeta.session_id == session_id)
            ).scalar_one_or_none()

            # if location data is unavailable
            if lat == "unknown" or long == "unknown":
                # if session metadata doesnt exist yet, make a new with with whatever we have
                if not session_meta:
                    result = tf.get_geometry(tz_name=timezone)
                    longs, lats = result[0][0]
                    long = str(sum(longs) / len(longs))
                    lat = str(sum(lats) / len(lats))

                    session.add(
                        SessionMeta(
                            session_id=session_id,
                            location=f"{lat},{long}",
                            timezone=timezone,
                            local_ip=local_ip,
                        )
                    )
                    session.commit()

                # if metadata entry already exists, only update lat and long if it doesnt match with the received timezone
                # we update it to a coordinate that's inside the timezone
                else:
                    coords = str(session_meta.location).split(",")
                    lat = coords[0]
                    long = coords[1]

                    tz = tf.timezone_at(lat=float(lat), lng=float(long))
                    if tz != timezone:
                        result = tf.get_geometry(tz_name=timezone)
                        longs, lats = result[0][0]
                        long = str(sum(longs) / len(longs))
                        lat = str(sum(lats) / len(lats))
                        session_meta.location = f"{lat},{long}"

                    session_meta.timezone = timezone
                    session_meta.local_ip = local_ip
                    session.add(session_meta)
                    session.commit()

            # if we do have the lat long data, either add it if entry doesnt exist, or update it
            else:
                if not session_meta:
                    session.add(
                        SessionMeta(
                            session_id=session_id,
                            location=f"{lat},{long}",
                            timezone=timezone,
                            local_ip=local_ip,
                        )
                    )
                    session.commit()
                else:
                    session_meta.location = f"{lat},{long}"
                    session_meta.timezone = timezone
                    session_meta.local_ip = local_ip
                    session.add(session_meta)
                    session.commit()

        return (f"{lat},{long}", timezone, local_ip)

    async def provide_weblink(self, link: str) -> None:
        if not self._client:
            print("ERROR: Client is not created")
            raise BleakDeviceNotFoundError(BLE_SERVICE_UUID)

        if not self._client.is_connected:
            print("WARN: Read/write attempt with BLE device failed")
            raise BleakError("Connection lost")

        if link == "":
            link = " "

        print("provided:", link)
        await self._client.write_gatt_char(BLE_WEBLINK_CHAR_UUID, link.encode())

    async def send_priority_data(self, session_id: str) -> None:
        if not self._client:
            print("ERROR: Client is not created")
            raise BleakDeviceNotFoundError(BLE_SERVICE_UUID)

        if not self._client.is_connected:
            print("WARN: Read/write attempt with BLE device failed")
            raise BleakError("Connection lost")

        with Session(engine) as session:
            record = session.execute(
                select(PriorityData).where(PriorityData.session_id == session_id)
            ).scalar_one_or_none()

            data = " "

            if record and record.appname != "" and record.title != "":
                data = f"{record.appname}|{record.title}"

            print("sent priority data:", data)
            await self._client.write_gatt_char(BLE_PRIORITY_CHAR_UUID, data.encode())

    async def start_lock(self) -> None:
        if not self._client:
            print("ERROR: Client is not created")
            raise BleakDeviceNotFoundError(BLE_SERVICE_UUID)

        if not self._client.is_connected:
            print("WARN: Read/write attempt with BLE device failed")
            raise BleakError("Connection lost")

        await self._client.write_gatt_char(BLE_LOCK_CHAR_UUID, b"\x01")

    async def stop_lock(self) -> None:
        if not self._client:
            print("ERROR: Client is not created")
            raise BleakDeviceNotFoundError(BLE_SERVICE_UUID)

        if not self._client.is_connected:
            print("WARN: Read/write attempt with BLE device failed")
            raise BleakError("Connection lost")

        await self._client.write_gatt_char(BLE_LOCK_CHAR_UUID, b"\x00")

    async def start_alert(self) -> None:
        if not self._client:
            print("ERROR: Client is not created")
            raise BleakDeviceNotFoundError(BLE_SERVICE_UUID)

        if not self._client.is_connected:
            print("WARN: Read/write attempt with BLE device failed")
            raise BleakError("Connection lost")

        await self._client.write_gatt_char(BLE_ALERT_CHAR_UUID, b"\x01")

    async def stop_alert(self) -> None:
        if not self._client:
            print("ERROR: Client is not created")
            raise BleakDeviceNotFoundError(BLE_SERVICE_UUID)

        if not self._client.is_connected:
            print("WARN: Read/write attempt with BLE device failed")
            raise BleakError("Connection lost")

        await self._client.write_gatt_char(BLE_ALERT_CHAR_UUID, b"\x00")

    async def store_dequeue(self, session_id: str) -> None:
        if not self._client:
            print("ERROR: Client is not created")
            raise BleakDeviceNotFoundError(BLE_SERVICE_UUID)

        if not self._client.is_connected:
            print("WARN: Read/write attempt with BLE device failed")
            raise BleakError("Connection lost")

        data = await self._client.read_gatt_char(BLE_ENQUEUE_CHAR_UUID)
        value = data.decode("utf-8")
        if value == " ":
            return

        await self._client.write_gatt_char(BLE_ENQUEUE_CHAR_UUID, b" ")

        print("ID:", session_id)
        print("Received:", value)

        parts = value.split("|")

        if len(parts) < 4:
            return

        appname = parts[0]
        title = parts[1]
        text = parts[2]
        dt = datetime.fromtimestamp(int(parts[3]) / 1000)

        with Session(engine) as session:
            session.add(
                NotificationData(
                    session_id=session_id,
                    appname=appname,
                    title=title,
                    text=text,
                    datetime=dt,
                )
            )
            session.commit()

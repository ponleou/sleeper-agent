from abc import ABC, abstractmethod
from datetime import datetime, time, timedelta
import asyncio
from zoneinfo import ZoneInfo
from bleak import BleakScanner, BleakClient
from bleak.exc import BleakDeviceNotFoundError, BleakError, BleakGATTProtocolError
import socket
from models import PriorityData, engine, SessionRecord, NotificationData, SessionMeta
from sqlalchemy.orm import Session
from sqlalchemy import select
from timezonefinder import TimezoneFinder
from typing import cast, override
import requests

BLE_SERVICE_UUID = "00000100-addd-43a2-b9cc-6c8adc8a7761"
BLE_SESSION_CHAR_UUID = "00000101-addd-43a2-b9cc-6c8adc8a7761"
BLE_ENQUEUE_CHAR_UUID = "00000110-addd-43a2-b9cc-6c8adc8a7761"
BLE_LOCK_CHAR_UUID = "00000120-addd-43a2-b9cc-6c8adc8a7761"
BLE_PRIORITY_CHAR_UUID = "00000130-addd-43a2-b9cc-6c8adc8a7761"
BLE_ALERT_CHAR_UUID = "00000140-addd-43a2-b9cc-6c8adc8a7761"
BLE_WEBLINK_CHAR_UUID = "00000150-addd-43a2-b9cc-6c8adc8a7761"
BLE_METADATA_CHAR_UUID = "00000160-addd-43a2-b9cc-6c8adc8a7761"


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


############# DATABASE FUNCTIONS #############


def get_newest_session_id() -> str:
    with Session(engine) as session:
        record = (
            session.query(SessionRecord)
            .order_by(SessionRecord.created_at.desc())
            .first()
        )

        if not record:
            return " "

        return record.session_id


def is_registered(session_id: str) -> bool:
    with Session(engine) as session:
        session_record = session.execute(
            select(SessionRecord).where(SessionRecord.session_id == session_id)
        ).scalar_one_or_none()

        if not session_record:
            session.add(SessionRecord(session_id=session_id))
            session.commit()
            return False

        return session_record.registered


def get_bedtime(session_id: str) -> time | None:
    with Session(engine) as session:
        record = session.execute(
            select(SessionRecord).where(SessionRecord.session_id == session_id)
        ).scalar_one()

        return record.bedtime


def get_timezone(session_id: str) -> str:
    with Session(engine) as session:
        record = session.execute(
            select(SessionRecord).where(SessionRecord.session_id == session_id)
        ).scalar_one()

        return record.meta.timezone


def get_risingtime(session_id: str) -> time:
    with Session(engine) as session:
        record = session.execute(
            select(SessionMeta).where(SessionMeta.session_id == session_id)
        ).scalar_one()
        timezone = record.timezone
        coords = record.location.split(",")
        lat = coords[0]
        long = coords[1]

        r = requests.get(
            f"https://api.sunrise-sunset.org/json?lat={lat}&lng={long}&tzid={timezone}"
        )

        if r.ok:
            data = cast(dict[str, str], r.json()["results"])
            return (
                datetime.strptime(data["sunrise"], "%I:%M:%S %p") - timedelta(hours=1)
            ).time()
        else:
            return datetime.strptime("5:00:00 AM", "%I:%M:%S %p").time()


#######################################

############# UTILS #############


def get_current_time(timezone: str) -> time:
    # r = requests.get(f"https://timeapi.io/api/time/current/zone?timeZone={timezone}")

    # if r.ok:
    #     data = cast(dict[str, str], r.json())
    #     return datetime.strptime(data["time"], "%H:%M").time()
    # else:
        return datetime.now(ZoneInfo(timezone)).time()


def get_routing_table_ip(client_ip: str) -> str:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect((client_ip, 80))
    ip = cast(tuple[str, int], s.getsockname())[0]
    s.close()
    return ip


def check_to_end(current: time, start: time, end: time) -> bool:
    today = datetime.today()
    current_dt = datetime.combine(today, current)
    start_dt = datetime.combine(today, start)
    end_dt = datetime.combine(today, end)

    if start_dt > end_dt:
        end_dt += timedelta(hours=24)

    return current_dt >= end_dt


def check_to_start(current: time, start: time, end: time) -> bool:
    # assume it means end is the day before, and start is the day after
    if end < start:
        return current > start or current < end

    # assume end and start is on the same day
    else:
        return current > start and current < end


#######################################


class State(ABC):
    @abstractmethod
    async def run(self, interface: BleInterface) -> State:
        pass


# when id is not detected and it is time to be active
class AlertState(State):
    """
    State when no ID is detected, and it is time to start
    Transitions to DetectedState if ID is detected
    Transitions to PollingState if it is no longer time to start
    Else, remain in AlertState

    Can be transitioned from PollingState only

    Task:
    - start alert
    """

    start_time: time
    end_time: time
    timezone: str
    execute_once: bool
    returning: bool

    def __init__(
        self,
        start_time: time,
        end_time: time,
        timezone: str,
    ):
        self.start_time = start_time
        self.end_time = end_time
        self.timezone = timezone
        self.execute_once = False
        self.returning = False

    @override
    async def run(self, interface: BleInterface) -> State:
        if self.returning:
            print("returning")
            await asyncio.sleep(1)  # FIXME: increase time

        print("ALERT STATE")

        if not self.execute_once:
            self.execute_once = True
            await interface.start_alert()

        id = await interface.get_identity()
        if id != " ":
            return DetectedState()

        current = get_current_time(self.timezone)
        if check_to_start(current, self.start_time, self.end_time):
            self.returning = True
            return self
        else:
            return PollingState(self.start_time, self.end_time, self.timezone)


class PollingState(State):
    """
    State when no ID is detected
    Transitions to DetectedState if ID is detected
    Transitions to AlertState if it is time to start
    Else, remain in PollingState

    Can be transitioned from all states (AlertState, DetectedState, and ActiveState)

    Task:
    - stop alert
    - stop lock
    """

    start_time: time | None
    end_time: time | None
    latest_id: str
    timezone: str
    is_registered: bool
    execute_once: bool
    returning: bool

    def __init__(
        self,
        start_time: time | None = None,
        end_time: time | None = None,
        timezone: str = ""
    ):
        self.start_time = start_time
        self.end_time = end_time
        self.latest_id = " "
        self.timezone = timezone
        self.is_registered = False
        self.execute_once = False
        self.returning = False

    @override
    async def run(self, interface: BleInterface) -> State:
        if self.returning:
            print("returning")
            await asyncio.sleep(1)  # FIXME: increase time

        if not self.execute_once:
            self.execute_once = True

            await interface.stop_alert()
            await interface.stop_lock()

        id = await interface.get_identity()
        if id != " ":
            return DetectedState()

        if self.latest_id == " ":
            self.latest_id = get_newest_session_id()

        print("POLLING STATE:", self.latest_id)

        if not self.is_registered:
            self.is_registered = is_registered(self.latest_id)
            self.returning = True
            return self

        if not self.start_time or not self.end_time:
            self.start_time = get_bedtime(self.latest_id)
            self.end_time = get_risingtime(self.latest_id)

            if self.start_time == None:
                raise ValueError(
                    f"POLLING STATE UNEXPECTED: latest {self.latest_id} is registered but has no bedtime"
                )

        if self.timezone == "":
            self.timezone = get_timezone(self.latest_id)

        current = get_current_time(self.timezone)
        if check_to_start(current, self.start_time, self.end_time):
            return AlertState(self.start_time, self.end_time, self.timezone)
        else:
            self.returning = True
            return self


class DetectedState(State):
    """
    State when ID is detected
    Transitions to PollingState when no ID is detected
    Transitions to ActiveState when it is time to start
    Else, remain in DetectedState

    Can be transitioned from all states (AlertState, PollingState, and ActiveState)

    Task:
    - stop alert
    - stop lock
    """

    execute_once: bool
    is_registered: bool
    start_time: time | None
    end_time: time | None
    timezone: str
    returning: bool

    def __init__(self):
        self.execute_once = False
        self.is_registered = False
        self.start_time = None
        self.end_time = None
        self.timezone = ""
        self.returning = False

    @override
    async def run(self, interface: BleInterface) -> State:
        if self.returning:
            print("returning")
            await asyncio.sleep(1)  # FIXME: increase time

        id = await interface.get_identity()
        if id == " ":
            return PollingState()

        print("DETECTED STATE:", id)

        # only runs once per identified session id
        if not self.execute_once:
            self.execute_once = True

            await interface.stop_alert()
            await interface.stop_lock()

            self.is_registered = is_registered(id)
            _, self.timezone, ip = await interface.store_session_metadata(id)

            # only provide the weblink (to the register web interface) if not registered
            if not self.is_registered:
                await interface.provide_weblink(
                    f"{get_routing_table_ip(ip)}:5000/register?id={id}"
                )

        if not self.is_registered:
            self.returning = True
            return self

        if not self.start_time or not self.end_time:
            self.start_time = get_bedtime(id)
            self.end_time = get_risingtime(id)

            if self.start_time == None:
                raise ValueError(
                    f"DETECTED STATE UNEXPECTED: {id} is registered but has no bedtime, this is not allowed"
                )

        if self.timezone == "":
            self.timezone = get_timezone(id)

        current = get_current_time(self.timezone)
        if check_to_start(current, self.start_time, self.end_time):
            return ActiveState()
        else:
            self.returning = True
            return self


# if there is an id detected
class ActiveState(State):
    """
    State when ID is detected, and it is time to start
    Tansitions to PollingState when ID is not detected
    Transitioned to DetectedState when it is no longer time to start
    Else, remain in ActiveState

    Can be transitioned from DetectedState only

    Task:
    - start lock
    - dequeue data
    """

    start_active_time: time | None
    end_time: time | None
    timezone: str
    execute_once: bool
    returning: bool

    def __init__(self):
        self.start_active_time = None
        self.end_time = None
        self.timezone = ""
        self.execute_once = False
        self.returning = False

    @override
    async def run(self, interface: BleInterface) -> State:
        if self.returning:
            print("returning")
            await asyncio.sleep(1)  # FIXME: increase time

        id = await interface.get_identity()
        if id == " ":
            return PollingState()

        print("ACTIVE STATE:", id)
        await interface.store_dequeue(id)
        
        if not self.execute_once:
            self.execute_once = True
            await interface.start_lock()
            await interface.send_priority_data(id)

        if self.timezone == "":
            self.timezone = get_timezone(id)

        current_time = get_current_time(self.timezone)
        if not self.start_active_time or not self.end_time:
            self.start_active_time = current_time
            self.end_time = get_risingtime(id)

        if check_to_end(current_time, self.start_active_time, self.end_time):
            return DetectedState()
        else:
            self.returning = True
            return self


class StateMachine:
    state: State
    interface: BleInterface

    def __init__(self, interface: BleInterface):
        self.state = PollingState()
        self.interface = interface

    async def start(self):
        while True:
            await asyncio.sleep(0.2)  # FIXME: increase time
            self.state = await self.state.run(self.interface)


async def main() -> None:
    interface = await BleInterface().create()
    print("connected")
    
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

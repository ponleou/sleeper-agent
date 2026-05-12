from __future__ import annotations
from abc import ABC, abstractmethod
from datetime import datetime, time, timedelta
import asyncio
from zoneinfo import ZoneInfo
from models import engine, SessionRecord, SessionMeta
from sqlalchemy.orm import Session
from sqlalchemy import select
from ble_interface import BleInterface
import socket
from typing import cast, override
import requests


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
    Transitions to ScanningState if it is no longer time to start
    Else, remain in AlertState

    Can be transitioned from ScanningState only

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
            await asyncio.sleep(2)
        else:
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
            return ScanningState(self.start_time, self.end_time, self.timezone)


class ScanningState(State):
    """
    State when no ID is detected
    Transitions to DetectedState if ID is detected
    Transitions to AlertState if it is time to start
    Else, remain in ScanningState

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
        timezone: str = "",
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
            await asyncio.sleep(3)

        if not self.execute_once:
            self.execute_once = True

            await interface.stop_alert()
            await interface.end_start_action()

        id = await interface.get_identity()
        if id != " ":
            return DetectedState()

        if self.latest_id == " ":
            self.latest_id = get_newest_session_id()

        if not self.returning:
            print("SCANNING STATE:", self.latest_id)

        if not self.is_registered:
            self.is_registered = is_registered(self.latest_id)
            self.returning = True
            return self

        if not self.start_time or not self.end_time:
            self.start_time = get_bedtime(self.latest_id)
            self.end_time = get_risingtime(self.latest_id)

            if self.start_time == None:
                raise ValueError(
                    f"SCANNING STATE UNEXPECTED: latest {self.latest_id} is registered but has no bedtime"
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
    Transitions to ScanningState when no ID is detected
    Transitions to ActiveState when it is time to start
    Else, remain in DetectedState

    Can be transitioned from all states (AlertState, ScanningState, and ActiveState)

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
            await asyncio.sleep(3)

        id = await interface.get_identity()
        if id == " ":
            return ScanningState()

        if not self.returning:
            print("DETECTED STATE:", id)

        # only runs once per identified session id
        if not self.execute_once:
            self.execute_once = True

            await interface.stop_alert()
            await interface.end_start_action()

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
    Tansitions to ScanningState when ID is not detected
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
            await asyncio.sleep(10)

        id = await interface.get_identity()
        if id == " ":
            return ScanningState()

        if not self.returning:
            print("ACTIVE STATE:", id)
            
        await interface.store_dequeue(id)

        if not self.execute_once:
            self.execute_once = True
            await interface.set_start_action()
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
        self.state = ScanningState()
        self.interface = interface

    async def start(self):
        while True:
            await asyncio.sleep(0.2)  # FIXME: increase time
            self.state = await self.state.run(self.interface)
            

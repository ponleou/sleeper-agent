from datetime import datetime, time
from sqlalchemy import (
    Time,
    create_engine,
    Integer,
    String,
    DateTime,
    ForeignKey,
    Boolean,
)
from sqlalchemy.orm import DeclarativeBase, Mapped, mapped_column, relationship
from typing import final


class Base(DeclarativeBase):
    pass


@final
class NotificationData(Base):
    __tablename__: str = "notification_data"
    id: Mapped[int] = mapped_column(Integer, primary_key=True)
    session_id: Mapped[str] = mapped_column(String, ForeignKey("sessions.session_id"))
    appname: Mapped[str] = mapped_column(String)
    title: Mapped[str] = mapped_column(String)
    text: Mapped[str] = mapped_column(String)
    datetime: Mapped[datetime] = mapped_column(DateTime)


@final
class SessionMeta(Base):
    __tablename__: str = "session_meta"
    session_id: Mapped[str] = mapped_column(
        String, ForeignKey("sessions.session_id"), primary_key=True
    )
    location: Mapped[str] = mapped_column(String)
    timezone: Mapped[str] = mapped_column(String)
    local_ip: Mapped[str] = mapped_column(String)


@final
class PriorityData(Base):
    __tablename__: str = "priority_data"
    session_id: Mapped[str] = mapped_column(String, ForeignKey("sessions.session_id"), primary_key=True)
    appname: Mapped[str] = mapped_column(String)
    title: Mapped[str] = mapped_column(String)


@final
class SessionRecord(Base):
    __tablename__: str = "sessions"
    session_id: Mapped[str] = mapped_column(String, primary_key=True)
    registered: Mapped[bool] = mapped_column(Boolean, default=False)
    bedtime: Mapped[time | None] = mapped_column(Time, nullable=True)
    created_at: Mapped[datetime] = mapped_column(DateTime, default=datetime.now)
    notifications: Mapped[list[NotificationData]] = relationship("NotificationData")
    meta: Mapped[SessionMeta] = relationship("SessionMeta", uselist=False)
    priority: Mapped[PriorityData] = relationship("PriorityData")

engine = create_engine("sqlite:///data/main.db")
Base.metadata.create_all(engine)

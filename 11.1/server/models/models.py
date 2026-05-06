from sqlalchemy import create_engine, Column, Integer, String, DateTime, ForeignKey, Boolean
from sqlalchemy.orm import DeclarativeBase, relationship
from typing import final


class Base(DeclarativeBase):
    pass


@final
class SessionRecord(Base):
    __tablename__: str = 'sessions'
    session_id = Column(String, primary_key=True)
    registered = Column(Boolean, default=False)
    notifications = relationship('NotificationData')
    meta = relationship('SessionMeta', uselist=False)
    priority = relationship('PriorityData')


@final
class NotificationData(Base):
    __tablename__: str = "notification_data"
    id = Column(Integer, primary_key=True)
    session_id = Column(String, ForeignKey("sessions.session_id"))
    appname = Column(String)
    title = Column(String)
    text = Column(String)
    datetime = Column(DateTime)


@final
class SessionMeta(Base):
    __tablename__: str = "session_meta"
    session_id = Column(String, ForeignKey("sessions.session_id"), primary_key=True)
    location = Column(String)
    timezone = Column(String)
    local_ip = Column(String)


@final
class PriorityData(Base):
    __tablename__: str = "priority_data"
    id = Column(Integer, primary_key=True)
    session_id = Column(String, ForeignKey("sessions.session_id"))
    appname = Column(String)
    title = Column(String)


engine = create_engine("sqlite:///data/main.db")
Base.metadata.create_all(engine)

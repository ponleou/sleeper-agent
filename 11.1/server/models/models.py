from sqlalchemy import create_engine, Column, Integer, String, DateTime, ForeignKey
from sqlalchemy.orm import DeclarativeBase, relationship
from typing import final

class Base(DeclarativeBase):
    pass

@final
class SessionRecord(Base):
    __tablename__: str = 'sessions'
    id = Column(Integer, primary_key=True)
    session_id = Column(String, unique=True)
    notifications = relationship('NotificationData', back_populates='session')
    meta = relationship('SessionMeta', back_populates='session', uselist=False)

@final
class NotificationData(Base):
    __tablename__: str = 'notification_data'
    id = Column(Integer, primary_key=True)
    session_id = Column(String, ForeignKey('sessions.session_id'))
    appname = Column(String)
    title = Column(String)
    text = Column(String)
    datetime = Column(DateTime)
    session = relationship('SessionRecord', back_populates='notifications')

@final
class SessionMeta(Base):
    __tablename__: str = 'session_meta'
    session_id = Column(String, ForeignKey('sessions.session_id'), primary_key=True)
    location = Column(String)
    timezone = Column(String)
    session = relationship('SessionRecord', back_populates='meta')

engine = create_engine('sqlite:///data/main.db')
Base.metadata.create_all(engine)
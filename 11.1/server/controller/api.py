from flask import Blueprint, redirect, request, url_for
from models import engine, SessionRecord, PriorityData
from sqlalchemy.orm import Session
from sqlalchemy import select
from datetime import time

api = Blueprint("api", __name__)

@api.route("/register", methods=["POST"])
def register():
    id = request.form.get("id")
    appname = request.form.get("appname")
    title = request.form.get("title")
    bedtime = request.form.get("bedtime")

    if id and bedtime:
        bedtime_time = time.fromisoformat(bedtime)

        with Session(engine) as session:
            record = session.execute(select(SessionRecord).where(SessionRecord.session_id == id)).scalar_one_or_none()

            if not record:
                return redirect(url_for('views.register', id=id))

            record.bedtime = bedtime_time
            record.registered = True

            if appname and title:
                priority = session.execute(select(PriorityData).where(PriorityData.session_id == id)).scalar_one_or_none()

                if not priority:
                    session.add(PriorityData(session_id=id, appname=appname.lower(), title=title.lower()))
                else:
                    priority.appname = appname.lower()
                    priority.title = title.lower()
            
            session.commit()

    return redirect(url_for('views.register', id=id))
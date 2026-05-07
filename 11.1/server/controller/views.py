from flask import Blueprint, render_template, request
from models import engine, SessionRecord
from sqlalchemy.orm import Session
from sqlalchemy import select

views = Blueprint("views", __name__)


@views.route("/register")
def register():
    id = request.args.get("id")
    with Session(engine) as session:
        record = session.execute(select(SessionRecord).where(SessionRecord.session_id == id)).scalar_one_or_none()
        if not record:
            id = None

    return render_template("register.html.j2", id=id)

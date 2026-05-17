from flask import Flask
from .views import views
from .api import api


def register_routes(app: Flask):
    app.register_blueprint(views)
    app.register_blueprint(api, url_prefix="/api")
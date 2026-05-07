from flask import Flask
from controller import register_routes


app = Flask(__name__, template_folder="views")
register_routes(app)

app.config["TEMPLATES_AUTO_RELOAD"] = True

if __name__ == "__main__":
    app.run(debug=True, use_reloader=True)
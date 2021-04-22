from flask import Flask, jsonify
from flask_swagger import swagger

app = Flask(__name__)


@app.route('/')
def hello_world():
    return 'Hello World!'


@app.route('/generate_game', methods=['GET'])
def generate_game():
    return 'Hello World!'


@app.route("/spec")
def spec():
    return jsonify(swagger(app))


def create_app():
    # existing code omitted

    from . import db
    db.init_app(app)

    return app


if __name__ == '__main__':
    create_app()

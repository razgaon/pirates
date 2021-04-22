from flask import Flask, jsonify
from flasgger import swag_from, Swagger
from core.game_generator import generate_game

app = Flask(__name__)
swagger = Swagger(app)


@app.route('/')
def hello_world():
    return 'Hello World!'


@app.route('/generate_game', methods=['GET'])
@swag_from('/docs/game.yml')
def generate_game():
    game = generate_game()


def create_app():
    # existing code omitted

    from . import db
    db.init_app(app)

    return app


if __name__ == '__main__':
    create_app()

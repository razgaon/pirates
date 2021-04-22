from flask import Flask
from flasgger import swag_from, Swagger
from core.game_generator import generate_game
from db.shared import db


def create_app():
    """
    Creates a new app and swagger instance.
    :return:
    """
    app = Flask(__name__)
    app.config['SQLALCHEMY_DATABASE_URI'] = f'sqlite:///db.sqlite'
    swagger = Swagger(app)
    db.init_app(app)

    return app, swagger


app, swagger = create_app()


# db.create_all(app=create_app()[0])


@app.route('/')
def hello_world():
    return 'Hello World!'


@app.route('/generate_game', methods=['GET'])
@swag_from('/docs/game.yml')
def generate_game():
    game = generate_game()


if __name__ == '__main__':
    create_app()

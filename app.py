from flask import Flask, jsonify
from flasgger import swag_from, Swagger
from core.game_generator import generate_game
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime

#https://flask-sqlalchemy.palletsprojects.com/en/2.x/quickstart/


app = Flask(__name__)
db = SQLAlchemy(app)
app.config['SQLALCHEMY_DATABASE_URI'] = f'sqlite:///db.sqlite'
swagger = Swagger(app)


class User(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), unique=True, nullable=False)

    def __repr__(self):
        return '<User %r>' % self.username
    
    def as_dict(self):
        return {c.name: getattr(self, c.name) for c in self.__table__.columns}


class Action(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, unique=True, nullable=False)
    action_type = db.Column(db.Integer, nullable=False)
    insertion_time = db.Column(db.DateTime, nullable=False, default=datetime.utcnow)

    def __repr__(self):
        return '<User %r>' % self.username


@app.route('/')
def hello_world():
    user = User.query.first()
    if not user:
        u = User(username='admin')
        db.session.add(u)
        db.session.commit()

    user = User.query.first()

    print(user)
    return jsonify(user.as_dict())


@app.route('/generate_game', methods=['GET'])
@swag_from('/docs/game.yml')
def generate_game():
    game = generate_game()


if __name__ == '__main__':
    app.run(debug=True)

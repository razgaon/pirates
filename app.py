from flask import Flask, jsonify
from flasgger import swag_from, Swagger
from core.game_generator import generate_game
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
import random

# https://flask-sqlalchemy.palletsprojects.com/en/2.x/quickstart/


app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = f'sqlite:///db.sqlite'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

db = SQLAlchemy(app)

swagger = Swagger(app)

task_archetypes = {"button-toggle": ['a-inator', 'b-inator', 'c-inator', 'd-inator'],
                   "button_increment": ['w-inator', 'x-inator', 'y-inator', 'z-inator'],
                   "button-LED-toggle": ['1-inator', '2-inator', '3-inator', '4-inator'],
                   "microphone-password": ['l-inator', 'm-inator', 'n-inator', 'o-inator', 'p-inator'],
                   "device-shake": ['fee', 'fi', 'fo', 'fum']}

task_texts = {"button-toggle": "Toggle the {control}",
              "button_increment": "Press the {control}",
              "button-LED-toggle": "Change the color of the {control}",
              "microphone-password": "Say the password into the {control}",
              "device-shake": "Shake the {control}!"}

NUM_PLAYERS = 2

"""
----------------------------------
DBS GO HERE
----------------------------------
"""


# class User(db.Model):
#     id = db.Column(db.Integer, primary_key=True)
#     username = db.Column(db.String(80), unique=True, nullable=False)

#     def __repr__(self):
#         return '<User %r>' % self.username

#     def as_dict(self):
#         return {c.name: getattr(self, c.name) for c in self.__table__.columns}


# class Action(db.Model):
#     id = db.Column(db.Integer, primary_key=True)
#     user_id = db.Column(db.Integer, unique=True, nullable=False)
#     action_type = db.Column(db.Integer, nullable=False)
#     insertion_time = db.Column(db.DateTime, nullable=False, default=datetime.utcnow)

#     def __repr__(self): 
#         return '<User %r>' % self.username

class Games(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    game_id = db.Column(db.String(10), nullable=False)
    player_id = db.Column(db.String(20), unique=True, nullable=False)
    # ready = db.Column(db.Boolean, nullable=False)
    score = db.Column(db.Integer, default=None)
    timestamp = db.Column(db.DateTime, default=None)

    def __repr__(self):
        return "TODO"


class Tasknames(db.Model):
    """
    Stores the mappings of task names to actual tasks
    """
    id = db.Column(db.Integer, primary_key=True)
    game_id = db.Column(db.String(10), nullable=False)
    player_id = db.Column(db.String(20), nullable=False)
    task_archetype = db.Column(db.String(20), nullable=False)
    task_name = db.Column(db.String(20), nullable=False)

    def __repr__(self):
        return "TODO"


class TaskAssignments(db.Model):
    """
    Stores which player is displaying which task
    """
    id = db.Column(db.Integer, primary_key=True)
    game_id = db.Column(db.String(10), nullable=False)
    player_id = db.Column(db.String(20), nullable=False)
    task_archetype = db.Column(db.String(20), nullable=False)
    task_name = db.Column(db.String(20), nullable=False)

    def __repr__(self):
        return "TODO"


class Outstanding(db.Model):
    """
    Stores which tasks are still open to being completed
    """
    id = db.Column(db.Integer, primary_key=True)
    game_id = db.Column(db.String(10), nullable=False)
    player_id = db.Column(db.String(20), nullable=False)
    task_archetype = db.Column(db.String(20), nullable=False)
    timestamp = db.Column(db.DateTime, nullable=False)

    def __repr__(self):
        return "TODO"


"""
----------------------------------
ENDPOINTS ARE BELOW
----------------------------------
"""


@app.route('/')
def hello_world():
    meta = db.metadata
    for table in reversed(meta.sorted_tables):
        db.session.execute(table.delete())
    db.session.commit()
    return "hello, world"


@app.route('/user_ready/<user>', methods=['POST'])
# Targeted (once per user) when a user is "shooting, should input their username into gamesDB"
def user_ready(user):
    # input into gamesDB the person's user and data for the specific gameID
    user = Games(game_id="game1", player_id=user)
    db.session.add(user)
    db.session.commit()
    # now we also want to generate some task names for them
    for archetype, possible_choices in task_archetypes.items():
        current_in_use = Tasknames.query.with_entities(Tasknames.task_name).filter_by(game_id="game1",
                                                                                      task_archetype=archetype).all()
        available = set(possible_choices) - set(current_in_use)
        mapping = Tasknames(game_id="game1", player_id=user.player_id, task_archetype=archetype,
                            task_name=random.sample(available, 1)[0])
        db.session.add(mapping)
        db.session.commit()
    # check if game has started!!!
    ready_players = Games.query.filter_by(game_id="game1").all()
    if len(ready_players) == NUM_PLAYERS:
        Games.query.filter(Games.game_id == "game1").update({"score": 0, "timestamp": datetime.now()})
        db.session.commit()
    return f"user {user.player_id} is now ready"


@app.route('/check_start/<game_id>/<user>', methods=['GET'])
# Targeted when a player is waiting for teh game to start
def check_start(game_id, user):
    assert game_id == "game1"
    # FIRST THING WE NEED TO DO: INSERT PLAYER INTO GAMESDB
    ready_players = Games.query.filter_by(game_id=game_id).all()
    if len(ready_players) == NUM_PLAYERS:
        # if we're in here, we're ready to start the game... we can send stuff down now
        mappings = Tasknames.query.with_entities(Tasknames.task_archetype, Tasknames.task_name).filter_by(
            game_id="game1", player_id=user).all()

        # get tasks that aren't assigned to the user (to be yelled)
        potential_tasks = Tasknames.query.with_entities(Tasknames.id, Tasknames.task_archetype,
                                                        Tasknames.task_name).filter(
            Tasknames.player_id != user).all()  # TODO: missing game1 filter, add here
        # only assign tasks that haven't been assigned yet
        used_ids = set(TaskAssignments.query.with_entities(TaskAssignments.id).all())
        response = "READY;"
        for id, archetype, name in potential_tasks:
            if id not in used_ids:
                # then we assign, and break
                assignment = TaskAssignments(id=id, game_id="game1", player_id=user, task_archetype=archetype,
                                             task_name=name)
                db.session.add(assignment)
                db.session.commit()

                task_text = task_texts[archetype].format(control=name)
                response += f"{task_text};"
                break

        for archetype, name in mappings:
            response += f"{archetype}:{name};"
        return response
    else:
        return "NOT READY"


@app.route('/generate_game', methods=['GET'])
@swag_from('/docs/game.yml')
def generate_game():
    game = generate_game()


if __name__ == '__main__':
    app.run(debug=True)

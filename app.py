from flask import Flask, request, jsonify
from flasgger import swag_from, Swagger
from core.game_generator import generate_game
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
import random

# https://flask-sqlalchemy.palletsprojects.com/en/2.x/quickstart/

#todo:
#generate goals and send with
#json returns
#task complete handling
#new round handing
#bigfixing: same player twice
#enter game code wikipedia

#-1 for unused controls
#else give goal

#round status running or over
#time limit for round -- never changes, even if all tasks complete

#nested JSON
#round status --> bool
#control: {name: skadoosh, goal: 10}
#more controls -- only send controls to display
#task: "turn on the flagoosh"


# JSON = {
#             status: true,
#             controllers:
#                 {
#                 button_incrementer:{name: “Scadoodle”, goal:10},
#                 button_toggle:{name”XXXX”, goal: -1},
#                 microphone:{name:"YYYY", goal: -1},
#                 shake:{name:"BBBBB", goal: -1}
#                     },
#               task: "Increment the scadoodle to 10",
#             }


#DIEGO -- for may3

#generate goals values and send them with the controls. -1 for a goal if that control will not be used for the round.



app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = f'sqlite:///db.sqlite'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

db = SQLAlchemy(app)

swagger = Swagger(app)

task_archetypes = {"button-toggle": ['a-inator', 'b-inator', 'c-inator', 'd-inator'],
                   "button-increment": ['w-inator', 'x-inator', 'y-inator', 'z-inator'],
                   "button-LED-toggle": ['1-inator', '2-inator', '3-inator', '4-inator'],
                   "microphone-password": ['l-inator', 'm-inator', 'n-inator', 'o-inator', 'p-inator'],
                   "device-shake": ['fee', 'fi', 'fo', 'fum']}

task_goals = {"button-toggle": [15, 30],
                   "button-increment": [30, 45],
                   "button-LED-toggle": [5, 8],
                   "microphone-password": [6, 10],
                   "device-shake": [2, 4]}

archetype_ix = {"button-toggle": 0,
                   "button-increment": 1,
                   "button-LED-toggle": 2,
                   "microphone-password": 3,
                   "device-shake": 4}

task_texts = {"button-toggle": "Toggle the {control}",
              "button-increment": "Press the {control}",
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


class TaskNameMappings(db.Model):
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

class CompletedTasks(db.Model):
    """
    Stores which tasks have been completed (same data entries as TaskAssignments)
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
    return "database cleared"


@app.route('/user_ready/<user>', methods=['POST'])
# Targeted (once per user) when a user is "shooting, should input their username into gamesDB"
def user_ready(user):
    # input into gamesDB the person's user and data for the specific gameID
    user = Games(game_id="game1", player_id=user)
    db.session.add(user)
    db.session.commit()
    # now we also want to generate some task names for them
    for archetype, possible_choices in task_archetypes.items():
        current_in_use = TaskNameMappings.query.with_entities(TaskNameMappings.task_name).filter_by(game_id="game1",
                                                                                      task_archetype=archetype).all()
        available = set(possible_choices) - set(current_in_use)
        mapping = TaskNameMappings(game_id="game1", player_id=user.player_id, task_archetype=archetype,
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
# Targeted when a player is waiting for the game to start
def check_start(game_id, user):
    assert game_id == "game1"
    # FIRST THING WE NEED TO DO: INSERT PLAYER INTO GAMESDB
    ready_players = Games.query.filter_by(game_id=game_id).all()
    if len(ready_players) == NUM_PLAYERS:
        # if we're in here, we're ready to start the game... we can send stuff down now

        #first begin by getting all the mappings, for each player, from a task archetype to a task name
        mappings = TaskNameMappings.query.with_entities(TaskNameMappings.task_archetype, TaskNameMappings.task_name).filter_by(game_id="game1", player_id=user).all()

        # get tasks that aren't assigned to the user (to be yelled)
        potential_tasks = TaskNameMappings.query.with_entities(TaskNameMappings.id, TaskNameMappings.task_archetype,
                                                        TaskNameMappings.task_name).filter(
            TaskNameMappings.player_id != user, TaskNameMappings.game_id == "game1").all()
        
        # only assign tasks to people that haven't been assigned yet
        used_ids = set(TaskAssignments.query.with_entities(TaskAssignments.id).all())
        response = {}
        response["status"] = True
        response["controllers"] = {}

        #in this section we want to send teh correct text to yell down to someone. We are indifferent as to whether they can get a task about themselves
        for id, archetype, name in potential_tasks:
            if id not in used_ids:
                # then we assign, and break
                assignment = TaskAssignments(id=id, game_id="game1", player_id=user, task_archetype=archetype,
                                             task_name=name)
                db.session.add(assignment)
                db.session.commit()

                response["task"] = task_texts[archetype].format(control=name)
                break
        
        #here, we communicate the mappings with the ESP
        ignore_lst = random.sample(range(0, len(mappings)-1), len(mappings)-4) if len(mappings) > 4 else []    
            cur_task_dict = {}
            cur_task_dict["controller_name"] = name
            cur_task_dict["controller_goal"] = random.randint(task_goals[archetype][0],task_goals[archetype][1])
            response["controllers"][archetype] = cur_task_dict
        return response
    else:
        return "NOT READY"

@app.route('/task_complete/<game_id>/<user>', methods=['POST'])
def task_complete(game_id, user):
    control_name = request.args["controller_name"]
    task = TaskAssignments.query.filter_by(task_archetype=control_name).first()
    completed_task = CompletedTasks(id=task.id, game_id=task.game_id, player_id=task.player_id, 
                                              task_archetype=task.task_archetype, task_name=task.task_name)
    db.session.add(completed_task)
    db.session.commit()
    count = CompletedTasks.query.count()
    if count == NUM_PLAYERS:



@app.route('/get_new_round/<game_id>/<user>', methods=['GET'])
def get_new_round(game_id, user):
    count = CompletedTasks.query.count()
    if NUM_PLAYERS > count:
        return jsonify({"status": "false"})
    else:

        # TODO: new round. clear TaskAssignments, clear CompletedTasks, clear Outstanding?, clear Tasknames, and re-generate



def generate_tasks():
    """
    This is the method that we will sue both in the check_start to initially assign tasks, 
    and then later in the generate_new_round to assign new tasks
    """




@app.route('/generate_game', methods=['GET'])
@swag_from('/docs/game.yml')
def generate_game():
    game = generate_game()


if __name__ == '__main__':
    app.run(debug=True)

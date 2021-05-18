import random
import json
import pytz
from datetime import datetime

from django.db.models import F
from django.http import JsonResponse, HttpResponse

from rest_framework.response import Response
from rest_framework.views import APIView

from .models import *
from .serializers import *
from rest_framework import viewsets, status
from rest_framework import permissions

task_archetypes = {"button-toggle": ['a-er', 'b-er', 'c-er', 'd-er'],
                   "button-LED-toggle": ['1-er', '2-er', '3-er', '4-er'],
                   "button-increment": ['w-er', 'x-er', 'y-er', 'z-er'],
                   "microphone-password": ['l-er', 'm-er', 'n-er', 'o-er', 'p-er'],
                   "device-shake": ['fee', 'fi', 'fo', 'fum']}

task_texts = {"button-toggle": "Toggle {control} [{num}] ",
              "button-LED-toggle": "Change {control} color [{num}]",
              "button-increment": "Press {control} [{num}]",
              "microphone-password": "Say {control} password [{num}]",
              "device-shake": "Shake {control} [{num}]"}

task_goals = {"button-toggle": [0, 2],
              "button-LED-toggle": [0, 3],
              "button-increment": [5, 15],
              "microphone-password": [1, 1],
              "device-shake": [1, 1]}

MAX_TIME = 180
utc = pytz.UTC


class GamesViewSet(viewsets.ModelViewSet):
    queryset = Games.objects.all()
    serializer_class = GamesSerializer


class TaskNameMappingsViewSet(viewsets.ModelViewSet):
    queryset = TaskNameMappings.objects.all()
    serializer_class = TaskNameMappingsSerializer


class TaskCommunicationViewSet(viewsets.ModelViewSet):
    queryset = TaskCommunication.objects.all()
    serializer_class = TaskCommunicationSerializer


class CurrentTasksViewSet(viewsets.ModelViewSet):
    queryset = CurrentTasks.objects.all()
    serializer_class = CurrentTasksSerializer


"""

META DATABASE MANIPULATION/LOOKUP METHODS

"""


class ClearGame(APIView):
    """
    Clears game DB, for testing purposes.
    """

    def get(self, request, format=None):
        TaskNameMappings.objects.all().delete()
        TaskCommunication.objects.all().delete()
        CurrentTasks.objects.all().delete()
        Games.objects.all().delete()
        return Response("data cleared!")


class GenerateGameCode(APIView):
    """
    Generates a game code for the frontend.
    """

    def get(self, request, format=None):
        current_ids = set(Games.objects.values_list('game_id', flat=True))
        x = random.randint(1000, 9999)
        while x in current_ids:
            x = random.randint(1000, 9999)
        return Response(x)


# can be used to fulfill Silvina requirement "3) Display the status of each task and update the score as tasks are completed.". If you also want score, then just call GameStatus or GetScore
class ViewTasks(APIView):
    """
    Returns the currently active set of tasks for the game with the provided game_id
    """

    def get(self, request, format=None):
        game_id = request.GET.get("game_id")
        base_json = {
            "status": "error",
            "text": "SERVER ERROR: DIDNT OVERWRITE JSON"
        }
        # check for bad stuff
        if game_id == None:
            base_json["text"] = "No game_id provided"
            return Response(base_json)
        elif CurrentTasks.objects.filter(game_id=game_id).count() < 1:
            base_json["text"] = "No current active tasks for this game OR game doesn't exist"
            return Response(base_json)

        # otherwise just query and send
        data = list(CurrentTasks.objects.filter(
            game_id=game_id).values())  # wrap in list(), because QuerySet is not JSON serializable
        return JsonResponse(data, safe=False)

    # can be used to fulfill Silvina requirement "2) Display all the tasks assigned to the ship group."


class ViewTaskComms(APIView):
    """
    Returns the current mappings of "task givers" to "task receivers"
    """

    def get(self, request, format=None):
        game_id = request.GET.get("game_id")
        base_json = {
            "status": "error",
            "text": "SERVER ERROR: DIDNT OVERWRITE JSON"
        }
        # check for bad stuff
        if game_id == None:
            base_json["text"] = "No game_id provided"
            return Response(base_json)
        elif TaskCommunication.objects.filter(game_id=game_id).count() < 1:
            base_json["text"] = "No current active tasks for this game OR game doesn't exist"
            return Response(base_json)

        # otherwise just query and send
        data = list(TaskCommunication.objects.filter(
            game_id=game_id).values())  # wrap in list(), because QuerySet is not JSON serializable
        return JsonResponse(data, safe=False)

    # {


#     'game_id': 1234
#     'status': 'in_progress',
#     'score': 15
# }
class ViewGame(APIView):
    """
    Returns the current state of the game with the provided game id
    """

    def get(self, request, format=None):
        game_id = request.GET.get("game_id")
        base_json = {
            "status": "error",
            "text": "SERVER ERROR: DIDNT OVERWRITE JSON"
        }
        # check for bad stuff
        if game_id == None:
            base_json["text"] = "No game_id provided"
            return Response(base_json)
        elif Games.objects.filter(game_id=game_id).count() == 0:
            base_json["text"] = "game doesn't exist"
            return Response(base_json)

        # otherwise just query and send
        response = generate_game_status_json(game_id)
        return Response(response)


class ViewLobby(APIView):
    """
    Returns the current state of the game with the provided game id
    """

    def get(self, request, format=None):
        resp = []
        for game_id in set(Games.objects.values_list('game_id', flat=True)):
            response = generate_game_status_json(game_id)
            resp.append(response)
        return Response(resp)


def generate_game_status_json(game_id):
    resp_inner = {}
    resp_inner["game_id"] = game_id
    resp_inner["players"] = [{"name":pid, "connected":conn} for pid, conn in Games.objects.filter(game_id=game_id).values_list("player_id", "esp_connected")]
    resp_inner["num_players"] = Games.objects.filter(game_id=game_id).first().num_players

    g_o = game_finished(game_id)
    g_s = game_started(game_id)

    resp_inner["status"] = "finished" if g_o else ("active" if g_s else "waiting for start")
    return resp_inner


"""

WEBSITE FACING METHODS

"""


class CreateGame(APIView):
    """
    Starts a new game with x number of players.
    """

    def get(self, request, format=None):
        game_id = request.GET.get("game_id")
        user_id = request.GET.get("user_id")
        number_of_players = request.GET.get("number_of_players")

        data = {'game_id': game_id, 'player_id': user_id, 'num_players': number_of_players, 'esp_connected': False}

        # Save a new game in the database
        game = Games.objects.filter(game_id=game_id).first()
        if game is not None:
            return Response(f"Game {game_id} has already been created!")
        else:

            player = Games(**data)
            player.save()

            return Response(
                f"Game {game_id} with {number_of_players} players generated by user {user_id}!")


class AddPlayer(APIView):
    """
    A player has sent a request that he is ready to start a game. Once all players are ready, the game can be started.
    """

    def get(self, request, format=None):
        # extract tags
        user_id = request.GET.get("user_id")
        game_id = request.GET.get("game_id")

        # check if game exists
        game = Games.objects.filter(game_id=game_id).first()
        if game is None:
            return Response(f"There is no game recorded with id {game_id}")
        
        # check if player in game
        game = Games.objects.filter(game_id=game_id, player_id=user_id).first()
        if game is not None:
            return Response(f"Player {user_id} has already joined game with id {game_id}")

        # check if game is full
        players_in_game = Games.objects.filter(game_id=game_id).count()
        game_size = Games.objects.filter(game_id=game_id).first().num_players
        if game_size == players_in_game: #we have gotten enough players already
            return Response(f"game {game_id} is already full")

        # if none of these happen, add player to game
        data = {'game_id': game_id,
                'player_id': user_id,
                'num_players': game_size,
                'esp_connected': False}
        newplayer = Games(**data)
        newplayer.save()

        return Response(f"user {user_id} is now ready")


class StartGame(APIView):
    """
    Starts the game with game_id.
    """

    def get(self, request, format=None):
        game_id = request.GET.get("game_id")

        # Check if all players are ready, and if so start the game.
        ready_players = Games.objects.filter(game_id=game_id, esp_connected=True)
        game_size = Games.objects.filter(game_id=game_id).first().num_players

        if len(ready_players) == game_size:
            self.begin_game(ready_players, game_id)
            return Response(f"Game {game_id} started!")
        else:
            return Response(f"Not enough players have connected their ESPs!")

    @staticmethod
    def begin_game(ready_players, game_id):
        ts = utc.localize(datetime.now())
        generate_round(ready_players, ts, game_id)
        # last, lets begin the game!
        Games.objects.filter(game_id=game_id).update(score=0, timestamp=ts, round_num=0)


class GetScore(APIView):
    """
    Returns score of game
    """

    def get(self, request, format=None):
        game_id = request.GET.get("game_id")
        if game_id == None:
            return Response("No game_id provided")
        else:
            game = Games.objects.filter(game_id=game_id).first()
            if game == None:
                return Response(f"There is no game recorded with id {game_id}")
            else:
                return Response(game.score)


# can be used to fulfill Silvina requirement "4) Indicate when the game is over and show the final score."
class GameStatus(APIView):
    """
    Returns score of game
    """

    def get(self, request, format=None):
        game_id = request.GET.get("game_id")

        game_over = game_finished(game_id)

        resp = {}
        if game_started(game_id) and game_over:
            resp["status"] = "finished"
            resp["score"] = Games.objects.filter(game_id=game_id).first().score
            return Response(resp)
        elif game_started(game_id) and not game_over:
            resp["status"] = "active"
            resp["score"] = Games.objects.filter(game_id=game_id).first().score
            return Response(resp)
        else:
            resp["status"] = "waiting for start"
            resp["score"] = -1
            return Response(resp)


def game_finished(game_id):
    ts = utc.localize(datetime.now())
    game_started_dt = Games.objects.filter(game_id=game_id).first().timestamp
    if game_started_dt is None:
        return False
    delta_time = (ts - game_started_dt).total_seconds()
    game_over = delta_time > MAX_TIME
    return game_over


def game_started(game_id):
    ready_players = Games.objects.filter(game_id=game_id, esp_connected=True)
    game_size = Games.objects.filter(game_id=game_id).first().num_players
    return len(ready_players) == game_size


"""

ESP FACING METHODS

"""


class CheckStart(APIView):
    """
    Check if the game has started. This request is sent periodically by the esp to see what the status of the game is.
    """

    def get(self, request, format=None):
        # extract tags
        user_id = request.GET.get("user_id")
        game_id = request.GET.get("game_id")

        print('got params')
        
        response = {
            "status": "error",
            "text": "SERVER ERROR: DIDNT OVERWRITE JSON"
        }
        # check for bad stuff
        if game_id == None:
            response["text"] = "No game_id provided"
            return Response(response)
        elif Games.objects.filter(game_id=game_id).count() < 1:
            print("no game with id")
            response["text"] = "game doesn't exist"
            return Response(response)

        print('no bad stuff')

        # check if all players have joined
        game_size = Games.objects.filter(game_id=game_id).first().num_players
        ready_players = Games.objects.filter(game_id=game_id, esp_connected=True)

        if len(ready_players) == game_size and CurrentTasks.objects.filter(game_id=game_id).count() == game_size:
            # if all players have joined, send ESP response
            fill_esp_response(response, user_id, game_id)
            return Response(response)
        else:
            # otherwise inform game has not started, but connect ESP
            Games.objects.filter(game_id=game_id, player_id=user_id).update(esp_connected=True)
            response["status"] = "static"
            response["text"] = "game has not started"
            return Response(response)


class TaskComplete(APIView):
    """
    Handles players logging a completed task
    """

    def post(self, request, format=None):
        # extract tags
        user_id = request.POST.get("user_id")
        game_id = request.POST.get("game_id")
        response = {
            "status": "error",
            "text": "SERVER ERROR: DIDNT OVERWRITE JSON"
        }
        
        # check if game exists
        game = Games.objects.filter(game_id=game_id).first()
        if game is None:
            return Response(f"There is no game recorded with id {game_id}")
        
        # check if player in game
        game = Games.objects.filter(game_id=game_id, player_id=user_id).first()
        if game is None:
            return Response(f"Player {user_id} is not in game with id {game_id}")

        # pull user task and check if they've repeated
        current_task = CurrentTasks.objects.filter(game_id=game_id, player_id=user_id)
        assert len(current_task) == 1, f"user {user_id} has a weird amount of tasks assigned"
        if current_task.first().finished:
            response["status"] = "error"
            response["text"] = f"task for user {user_id} had already been logged, no change"
            return Response(response)

        # if not, check whether the game is over, and if its not, update the task to be finished
        ts = utc.localize(datetime.now())
        game_started_dt = Games.objects.filter(game_id=game_id).first().timestamp
        # game_started_dt = datetime.strptime(game_started_str, '%Y-%m-%d %H:%M:%S.%f')
        delta_time = (ts - game_started_dt).total_seconds()

        if delta_time > MAX_TIME:
            response["status"] = "over"
            response["text"] = f"task was completed after the game was over"
            return Response(response)
        else: #game still going, update task to finished first
            CurrentTasks.objects.filter(game_id=game_id, player_id=user_id).update(finished=True)

            # calculate score for task completion and add to player score
            time_assigned, goal, archetype = \
                CurrentTasks.objects.values_list('timestamp', 'goal', 'task_archetype').filter(game_id=game_id,
                                                                                            player_id=user_id)[0]
            add_score = score_fn(utc.localize(datetime.now()), time_assigned, goal, archetype)
            Games.objects.filter(game_id=game_id).update(score=F('score') + add_score)

            # check number of finished tasks against the number of people in game... if same then new round!
            finished_tasks = CurrentTasks.objects.filter(game_id=game_id, finished=True)
            game_size = Games.objects.filter(game_id=game_id).first().num_players

            if len(finished_tasks) == game_size:
                # wipe databases with round-level info
                self.clean_tasks(game_id)

                # generate a new round and update the round counter
                self.generate_new_round(game_id)

            response["status"] = "update"
            response["text"] = f"task for user {user_id} succesfully logged"
            return Response(response)

    @staticmethod
    def clean_tasks(game_id):
        TaskNameMappings.objects.filter(game_id=game_id).delete()
        TaskCommunication.objects.filter(game_id=game_id).delete()
        CurrentTasks.objects.filter(game_id=game_id).delete()

    @staticmethod
    def generate_new_round(game_id):
        ready_players = Games.objects.filter(game_id=game_id).all()
        generate_round(ready_players, utc.localize(datetime.now()), game_id)
        Games.objects.filter(game_id=game_id).update(round_num=F('round_num') + 1)


class GetNewRound(APIView):
    """
    Returns some key info about mappings for the next round.
    """

    def get(self, request, format=None):
        # extract tags
        user_id = request.GET.get("user_id")
        game_id = request.GET.get("game_id")
        user_round_num = int(request.GET.get("round_num"))
        assert user_round_num is not None, "ESP transmitting user_round_num wrong"
        response = {
            "status": "error",
            "text": "SERVER ERROR: DIDNT OVERWRITE JSON"
        }
        
        # check if game exists
        game = Games.objects.filter(game_id=game_id).first()
        if game is None:
            return Response(f"There is no game recorded with id {game_id}")
        
        # check if player in game
        game = Games.objects.filter(game_id=game_id, player_id=user_id).first()
        if game is None:
            return Response(f"Player {user_id} is not in game with id {game_id}")

        # check whether the game is over, and if it is then send back a response
        ts = utc.localize(datetime.now())
        game_started_dt = Games.objects.filter(game_id=game_id).first().timestamp
        # game_started_dt = datetime.strptime(game_started_str, '%Y-%m-%d %H:%M:%S.%f')
        delta_time = (ts - game_started_dt).total_seconds()
        if delta_time > MAX_TIME:
            response["status"] = "over"
            response["text"] = f"Game Over!! Your team scored: {Games.objects.filter(game_id=game_id).first().score}"
            return Response(response)

        # get the theoretical game round number. If our game number is highewr than the user's number, then we have started a new round and they need info
        game_round_num = Games.objects.values_list('round_num', flat=True).filter(game_id=game_id)[0]
        if user_round_num < game_round_num:
            # The game has reassigned tasks and moved on to next round, pass this to the ESP
            fill_esp_response(response, user_id, game_id)
            return Response(response)
        else:
            # The round is still in play, nothing to update
            assert user_round_num == game_round_num, "user_round_num greater than game_round_num: Shouldn't happen"
            response["status"] = "static"
            response["text"] = "all tasks in round have not been completed yet"
            return Response(response)


def generate_name_mappings(game_id, ready_players, local_mappings):
    for archetype, name_options in task_archetypes.items():
        rndm_options = random.sample(name_options, len(name_options))
        rndm_options = rndm_options[:len(ready_players)]
        for i, player_obj in enumerate(ready_players):
            data = {'game_id': game_id,
                    'player_id': player_obj.player_id,
                    'task_archetype': archetype,
                    'task_name': rndm_options[i]}
            mapping = TaskNameMappings(**data)
            mapping.save()
            local_mappings[player_obj.player_id][archetype] = rndm_options[i]


def generate_round(ready_players, ts, game_id):
    """
    Actually GENERATES a new round. Makes assignments, saves to dbs
    """
    local_mappings = {k.player_id: {} for k in ready_players}
    # first, lets generate name mappings
    generate_name_mappings(game_id, ready_players, local_mappings)

    num_players = len(ready_players)
    # second, lets assign everyone tasks and communications
    possible_tasks = list(task_archetypes.keys())
    assigned_task_archetypes = [random.sample(possible_tasks, 1)[0] for _ in range(num_players)]
    order = random.sample([i for i in range(num_players)], num_players)
    for i, player_obj in enumerate(ready_players):
        # first, we assign the task to the person
        arch = assigned_task_archetypes[i]
        goal = random.randint(task_goals[arch][0], task_goals[arch][1])
        data = {'game_id': game_id,
                'player_id': player_obj.player_id,
                'task_archetype': arch,
                'goal': goal,
                'finished': False,
                'timestamp': ts}
        curtask = CurrentTasks(**data)
        curtask.save()
    for i, player_obj in enumerate(ready_players):
        # now, lets establish communication expectations
        my_id = player_obj.player_id
        target_player_order = order[(order.index(i) + 1) % len(order)]
        target_player_id = ready_players[target_player_order].player_id
        target_player_goal, task_archetype = \
            CurrentTasks.objects.values_list('goal', 'task_archetype').filter(game_id=game_id,
                                                                              player_id=target_player_id)[
                0]
        task_name = local_mappings[target_player_id][task_archetype]
        text = task_texts[task_archetype].format(control=task_name, num=target_player_goal)
        data = {'game_id': game_id,
                'speaker_player_id': my_id,
                'listener_player_id': target_player_id,
                'task_archetype': task_archetype,
                'task_name': task_name,
                'text': text}
        taskcomm = TaskCommunication(**data)
        taskcomm.save()


def fill_esp_response(response, user_id, game_id):
    """
    Uses pre-GENERATED data to FILL up a JSON response in the way the ESP likes it
    :param response:
    :param user_id:
    :param game_id:
    :return:
    """
    response["status"] = "update"
    response["round_num"] = Games.objects.values_list('round_num', flat=True).filter(game_id=game_id)[0]
    response["controllers"] = {}
    response["text"] = \
        TaskCommunication.objects.values_list('text', flat=True).filter(game_id=game_id, speaker_player_id=user_id)[0]

    # The task assigned to the specific user.
    assigned_task = CurrentTasks.objects.filter(game_id=game_id, player_id=user_id).first()

    required_ix = list(task_archetypes.keys()).index(assigned_task.task_archetype)
    ixs = set([i for i in range(len(task_archetypes.items()))]) - {required_ix}
    valid_ixs = random.sample(list(ixs), 3) + [required_ix]
    gui_quadrant_ixs = {0, 1, 2, 3}
    for i, (archetype, name_options) in enumerate(task_archetypes.items()):
        response["controllers"][archetype] = {}
        response["controllers"][archetype]["controller_name"] = \
            TaskNameMappings.objects.values_list('task_name', flat=True).filter(game_id=game_id, player_id=user_id,
                                                                                task_archetype=archetype)[0]
        response["controllers"][archetype]["controller_goal"] = -1 if i != required_ix else assigned_task.goal
        response["controllers"][archetype]["number"] = set.pop(gui_quadrant_ixs) if i in valid_ixs else -1


def score_speed(time_diff):
    return int(60 * (19 / 20) ** time_diff)


def score_difficulty(goal, archetype):
    # We give half points to the button LED toggle always -- the difficulty is the same
    if archetype == "button-LED-toggle":
        return 30
    else:
        archetype_max = task_goals[archetype][1]
        return int(20 + 20 * goal / archetype_max)


def score_fn(time_now, assign_time_str, goal, archetype):
    """
    Score breakdown: maximum 100 points
    40 points: linear scaling of task difficulty based on goal and range, transformed into [20, 40]
    60 points: decay function based on speed to completion. instantaneous = 60 points. 1 minute = 30 points.
    """
    assign_time = datetime.strptime(str(assign_time_str), '%Y-%m-%d %H:%M:%S.%f%z')
    return score_speed((time_now - assign_time).total_seconds()) + score_difficulty(goal, archetype)

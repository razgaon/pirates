import random
import json
from datetime import datetime

from django.db.models import F

from rest_framework.response import Response
from rest_framework.views import APIView

from .models import *
from .serializers import *
from rest_framework import viewsets, status
from rest_framework import permissions

task_archetypes = {"button-toggle": ['a-inator', 'b-inator', 'c-inator', 'd-inator'],
                   "button-LED-toggle": ['1-inator', '2-inator', '3-inator', '4-inator'],
                   "button-increment": ['w-inator', 'x-inator', 'y-inator', 'z-inator'],
                   "microphone-password": ['l-inator', 'm-inator', 'n-inator', 'o-inator', 'p-inator'],
                   "device-shake": ['fee', 'fi', 'fo', 'fum']}

task_texts = {"button-toggle": "Toggle the {control} {num} times",
              "button-LED-toggle": "Change the color of the {control} to {num}",
              "button-increment": "Press the {control} {num} times",
              "microphone-password": "Say the password into the {control} {num} times",
              "device-shake": "Shake the {control} {num} time!"}

task_goals = {"button-toggle": [0, 2],
              "button-LED-toggle": [0, 3],
              "button-increment": [5, 15],
              "microphone-password": [1, 1],
              "device-shake": [1, 1]}

MAX_TIME = 60

base_json = {
    "status": "error",
    "text": "SERVER ERROR: DIDNT OVERWRITE JSON"
}


class GamesViewSet(viewsets.ModelViewSet):
    """
    API endpoint that allows groups to be viewed or edited.
    """
    queryset = Games.objects.all()
    serializer_class = GamesSerializer


class TaskNameMappingsViewSet(viewsets.ModelViewSet):
    """
    API endpoint that allows users to be viewed or edited.
    """
    queryset = TaskNameMappings.objects.all()
    serializer_class = TaskNameMappingsSerializer


class TaskCommunicationViewSet(viewsets.ModelViewSet):
    """
    API endpoint that allows groups to be viewed or edited.
    """
    queryset = TaskCommunication.objects.all()
    serializer_class = TaskCommunicationSerializer


class CurrentTasksViewSet(viewsets.ModelViewSet):
    """
    API endpoint that allows groups to be viewed or edited.
    """
    queryset = CurrentTasks.objects.all()
    serializer_class = CurrentTasksSerializer


class ClearGame(APIView):
    """
    Clears game DB, for testing purposes.
    """

    def get(self, request, format=None):
        TaskNameMappings.objects.all().delete()
        TaskCommunication.objects.all().delete()
        CurrentTasks.objects.all().delete()
        Games.objects.all().delete()
        return Response("data cleared!") #no json because ESP should never request a clear


class Score(APIView):
    """
    Returns score of game
    """

    def get(self, request, format=None):
        game_id = request.GET.get("game_id")
        if game_id == None:
            base_json["text"] = "No game_id provided"
        else:
            game = Games.objects.filter(game_id=game_id).first()
            if game == None:
                base_json["text"] = f"There is no game recorded with id {game_id}"
            else:
                base_json["status"] = "update"
                base_json["text"] = f"The score for game with id {game_id} is {game.score}"
        return Response(base_json)


class PlayerReady(APIView):
    """
    List all snippets, or create a new snippet.
    """

    def post(self, request, format=None):
        # extract tags
        user_id = request.POST.get("user_id")
        game_id = request.POST.get("game_id")
        
        #update player to ready, and check if enough players have joined
        Games.objects.filter(game_id=game_id, player_id=user_id).update(ready=True)
        game_size = Games.objects.values_list('num_players', flat=True).filter(game_id=game_id)[0]
        ready_players = Games.objects.filter(game_id=game_id, ready=True)
        
        if len(ready_players) == game_size:
            ts = datetime.now()
            generate_round(ready_players, ts, game_id)
            # last, lets begin the game!
            Games.objects.filter(game_id=game_id).update(score=0, timestamp=ts, round_num=0)
        base_json["status"] = "update"
        base_json["text"] = f"user {user_id} is now ready"
        return Response(base_json)

# data = {'game_id': game_id, 'player_id': user_id}
# player = Games(**data)
# player.save()

class CheckStart(APIView):
    """
    List all snippets, or create a new snippet.
    """

    def get(self, request, format=None):
        # extract tags
        user_id = request.GET.get("user_id")
        game_id = request.GET.get("game_id")
        
        #check if all players have joined
        game_size = Games.objects.values_list('num_players', flat=True).filter(game_id=game_id)[0]
        ready_players = Games.objects.filter(game_id=game_id, ready=True)
        response = base_json
        
        if len(ready_players) == game_size:
            #if all players have joined, assign everything and send ESP response
            fill_esp_response(response, user_id, game_id)
            return Response(response)
        else:
            #otherwise inform game has not started
            response["status"] = "static"
            response["text"] = "game has not not started"
            return Response(response)


class TaskComplete(APIView):
    """
    Handles players logging a completed task
    """

    def post(self, request, format=None):
        #extract tags
        user_id = request.POST.get("user_id")
        game_id = request.POST.get("game_id")
        
        #pull user task and check if they've repeated
        current_task = CurrentTasks.objects.filter(game_id=game_id, player_id=user_id)
        assert len(current_task) == 1 # A player can't have more than one task at a time
        if current_task.first().finished:
            response = base_json
            response["status"] = "error"
            response["text"] = f"task for user {user_id} had already been logged, no change"
            return Response(response)
        
        #if not, check whether the game is over, and if its not, update the task to be finished
        ts = dateteime.now()
        game_started_str = CurrentGamesTasks.objects.values_list('timestamp', flat=True).filter(game_id=game_id)[0]
        game_started_dt = datetime.strptime(game_started_str, '%Y-%m-%d %H:%M:%S.%f')
        delta_time = (ts-game_started_dt).total_seconds()
        if delta_time > MAX_TIME:
            response["status"] = "error"
            response["text"] = f"task for user {user_id} had already been logged, no change"
        CurrentTasks.objects.filter(game_id=game_id, player_id=user_id).update(finished=True)

        #calculate score for task completion and add to player score
        time_assigned, goal, archetype = \
            CurrentTasks.objects.values_list('timestamp', 'goal', 'task_archetype').filter(game_id=game_id, player_id=user_id)[0]
        add_score = score_fn(datetime.now(), time_assigned, goal, archetype)
        Games.objects.filter(game_id=game_id).update(score=F('score') + add_score)
        
        #check number of finished tasks against the number of people in game... if same then new round!
        finished_tasks = CurrentTasks.objects.filter(game_id=game_id, finished=True)
        game_size = Games.objects.values_list('num_players', flat=True).filter(game_id=game_id)[0]
        
        if len(finished_tasks) == game_size:
            #wipe databases with round-level info
            TaskNameMappings.objects.filter(game_id=game_id).delete()
            TaskCommunication.objects.filter(game_id=game_id).delete()
            CurrentTasks.objects.filter(game_id=game_id).delete()
            
            #generate a new round and update the round counter
            ready_players = Games.objects.filter(game_id=game_id).all()
            generate_round(ready_players, datetime.now(), game_id)
            Games.objects.filter(game_id=game_id).update(round_num=F('round_num') + 1)
        
        base_json["status"] = "update"
        base_json["text"] = f"task for user {user_id} succesfully logged"
        return Response(base_json)


class GetNewRound(APIView):
    """
    Returns some key info about mappings for the next round.
    """

    def get(self, request, format=None):
        #extract tags
        user_id = request.GET.get("user_id")
        game_id = request.GET.get("game_id")
        user_round_num = int(request.GET.get("round_num"))
        assert user_round_num != None, "ESP transmitting user_round_num wrong"

        #get the theoretical game round number. If our game number is highewr than the user's number, then we have started a new round and they need info
        game_round_num = Games.objects.values_list('round_num', flat=True).filter(game_id=game_id)[0]
        response = base_json

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


class GenerateGameCode(APIView):
    """
    Generates a game code
    """

    def get(self, request, format=None):
        current_ids = set(Games.objects.values_list('game_id', flat=True))
        x = random.randint(1000, 9999)
        while x in current_ids:
            x = random.randint(1000, 9999)
        return Response(x)


def generate_round(ready_players, ts, game_id):
    """
    Actually GENERATES a new round. Makes assignments, saves to dbs
    """
    num_players = len(ready_players)
    local_mappings = {k.player_id:{} for k in ready_players}
    # first, lets generate name mappings
    for archetype, name_options in task_archetypes.items():
        rndm_options = random.sample(name_options, len(name_options))
        rndm_options = rndm_options[:num_players]
        for i, player_obj in enumerate(ready_players):
            data = {'game_id': game_id,
                    'player_id': player_obj.player_id,
                    'task_archetype': archetype,
                    'task_name': rndm_options[i]}
            mapping = TaskNameMappings(**data)
            mapping.save()
            local_mappings[player_obj.player_id][archetype] = rndm_options[i]
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
        target_player_goal, task_archetype = CurrentTasks.objects.values_list('goal', 'task_archetype').filter(game_id=game_id, player_id=target_player_id)[0]
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
    """
    response["status"] = "update"
    response["round_num"] = Games.objects.values_list('round_num', flat=True).filter(game_id=game_id)[0]
    response["controllers"] = {}
    response["text"] = \
        TaskCommunication.objects.values_list('text', flat=True).filter(game_id=game_id, speaker_player_id=user_id)[0]
    assigned_task = CurrentTasks.objects.filter(game_id=game_id, player_id=user_id).first()
    required_ix = list(task_archetypes.keys()).index(assigned_task.task_archetype)
    ixs = set([i for i in range(len(task_archetypes.items()))]) - set([required_ix])
    valid_ixs = random.sample(list(ixs), 2) + [required_ix]
    gui_quadrant_ixs = {0, 1, 2 }
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
    assign_time = datetime.strptime(assign_time_str, '%Y-%m-%d %H:%M:%S.%f')
    return score_speed((time_now-assign_time).total_seconds()) + score_difficulty(goal, archetype)

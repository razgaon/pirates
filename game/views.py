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
                   "button_increment": ['w-inator', 'x-inator', 'y-inator', 'z-inator'],
                   "button-LED-toggle": ['1-inator', '2-inator', '3-inator', '4-inator'],
                   "microphone-password": ['l-inator', 'm-inator', 'n-inator', 'o-inator', 'p-inator'],
                   "device-shake": ['fee', 'fi', 'fo', 'fum']}

task_texts = {"button-toggle": "Toggle the {control} {num} times",
              "button_increment": "Press the {control} {num} times",
              "button-LED-toggle": "Change the color of the {control} {num} times",
              "microphone-password": "Say the password into the {control} {num} times",
              "device-shake": "Shake the {control} {num} times!"}

task_goals = {"button-toggle": [0, 2],
                   "button-increment": [5, 15],
                   "button-LED-toggle": [0, 3],
                   "microphone-password": [1, 1],
                   "device-shake": [1, 1]}

NUM_PLAYERS = 2


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
        game_id = request.GET.get("game_id")
        # assert game_id == "game1"

        TaskNameMappings.objects.filter(game_id=game_id).delete()
        TaskCommunication.objects.filter(game_id=game_id).delete()
        CurrentTasks.objects.filter(game_id=game_id).delete()
        Games.objects.filter(game_id=game_id).delete()
        return Response("data cleared")

class Score(APIView):
    """
    Returns score of game
    """

    def get(self, request, format=None):
        game_id = request.GET.get("game_id")
        if game_id == None:
            return Response("Please give game id")

        game = Games.objects.filter(game_id=game_id).first()
        if game == None:
            return Response(f"There is no game recorded with id {game_id}")
        else:
            return Response(f"The score for game with id {game_id} is {game.score}")


class PlayerReady(APIView):
    """
    List all snippets, or create a new snippet.
    """

    def post(self, request, format=None):
        # First add the user.
        user_id = request.POST.get("user_id")
        game_id = request.POST.get("game_id")
        data = {'game_id': game_id, 'player_id': user_id}
        player = Games(**data)
        player.save()
        # IF WE NOW HAVE THE COMPLETE GAME, WE'RE GOING TO DO A BUNCH OF STUFF
        ready_players = Games.objects.filter(game_id=game_id)
        if len(ready_players) == NUM_PLAYERS:
            ts = datetime.now()
            generate_round(ready_players, ts, game_id)
            #last, lets begin the game!
            Games.objects.filter(game_id=game_id).update(score=0, timestamp=ts, round_num=0)

        return Response(f"user {user_id} is now ready", status=status.HTTP_200_OK)


class CheckStart(APIView):
    """
    List all snippets, or create a new snippet.
    """

    def get(self, request, format=None):
        user_id = request.GET.get("user_id")
        game_id = request.GET.get("game_id")

        assert game_id == "game1"
        ready_players = Games.objects.filter(game_id=game_id).all()
        response = {}
        if len(ready_players) == NUM_PLAYERS:
            generate_esp_response(response, user_id, game_id)
            return Response(response)
        else:
            response["status"] = False
            return Response(json.dumps(response))

class TaskComplete(APIView):
    """
    Handles players logging a completed task
    """
    def post(self, request, format=None):
        user_id = request.POST.get("user_id")
        game_id = request.POST.get("game_id")

        CurrentTasks.objects.filter(game_id=game_id, player_id=user_id).update(finished=True)
        time_assigned = CurrentTasks.objects.values_list('timestamp', flat=True).filter(game_id=game_id,player_id=user_id)[0]
        goal = CurrentTasks.objects.values_list('goal', flat=True).filter(game_id=game_id,player_id=user_id)[0]
        score = ritaank_func(datetime.now(), time_assigned, goal)
        Games.objects.filter(game_id=game_id).update(score=score)
        finished_tasks = CurrentTasks.objects.filter(game_id=game_id, finished=True)
        if len(finished_tasks) == NUM_PLAYERS:
            TaskNameMappings.objects.filter(game_id=game_id).delete()
            TaskCommunication.objects.filter(game_id=game_id).delete()
            CurrentTasks.objects.filter(game_id=game_id).delete()
            ready_players = Games.objects.filter(game_id=game_id).all()
            generate_round(ready_players, datetime.now(), game_id)
            Games.objects.filter(game_id=game_id).update(round_num=F('round_num')+1)
        return Response(f"task for user {user_id} succesfully logged", status=status.HTTP_200_OK)


class GetNewRound(APIView):
    """
    Returns some key info about mappings for the next round.
    """
    def get(self, request, format=None):
        user_id = request.GET.get("user_id")
        game_id = request.GET.get("game_id")
        user_round_num = int(request.GET.get("round_num"))
        assert user_round_num != None

        game_round_num = Games.objects.values_list('round_num', flat=True).filter(game_id=game_id)[0]
        assert game_id == "game1"
        response = {}
        # assert 1==2, f"{user_round_num} \t\t {game_round_num} \t\t {type(user_round_num)} \t\t {type(game_round_num)}"
        if user_round_num < game_round_num:
            # The game has reassigned tasks and moved on to next round, pass this to the ESP
            generate_esp_response(response, user_id, game_id)
            return Response(response)
        else:
            # The round is still in play, nothing to update
            assert user_round_num == game_round_num
            response["status"] = False
            return Response(json.dumps(response))


def generate_round(ready_players, ts, game_id):
    local_mappings = {}
    #first, lets generate name mappings
    for archetype, name_options in task_archetypes.items():
        rndm_options = random.sample(name_options, len(name_options))
        rndm_options = rndm_options[:NUM_PLAYERS]
        for i, player_obj in enumerate(ready_players):
            data = {'game_id': game_id,
                    'player_id': player_obj.player_id,
                    'task_archetype': archetype,
                    'task_name': rndm_options[i]}
            mapping = TaskNameMappings(**data)
            mapping.save()
            local_mappings[player_obj.player_id] = (archetype, rndm_options[i])
    #second, lets assign everyone tasks and communications
    possible_tasks = list(task_archetypes.keys())
    assigned_task_archetypes = [random.sample(possible_tasks, 1) for _ in range(NUM_PLAYERS)]
    order = random.sample([i for i in range(NUM_PLAYERS)], NUM_PLAYERS)
    for i, player_obj in enumerate(ready_players):
        #first, we assign the task to the person
        arch = local_mappings[player_obj.player_id][0]
        goal = random.randint(task_goals[arch][0], task_goals[arch][1])
        data = {'game_id': game_id,
                'player_id': player_obj.player_id,
                'task_archetype': arch,
                'goal': goal,
                'finished':False,
                'timestamp': ts}
        curtask = CurrentTasks(**data)
        curtask.save()
        #now, lets establish communication expectations
        my_id = player_obj.player_id
        # print(order, i, order.index(i))
        try:
            target_player_order = order[(order.index(i)+1) % len(order)]
        except IndexError:
            print((order.index(i)+1) % len(order))
            raise AssertionError
        target_player_id = ready_players[target_player_order].player_id
        task_archetype = local_mappings[target_player_id][0]
        task_name = local_mappings[target_player_id][1]
        text = task_texts[task_archetype].format(control=task_name, num=goal)
        data = {'game_id': game_id,
                'speaker_player_id': my_id,
                'listener_player_id': target_player_id,
                'task_archetype': task_archetype,
                'task_name': task_name,
                'text': text}
        taskcomm = TaskCommunication(**data)
        taskcomm.save()


def generate_esp_response(response, user_id, game_id):
    response["status"] = True
    response["round_num"] = Games.objects.values_list('round_num', flat=True).filter(game_id=game_id)[0]
    response["controllers"] = {}
    response["text"] = TaskCommunication.objects.values_list('text', flat=True).filter(game_id=game_id,speaker_player_id=user_id)[0]
    assigned_task = CurrentTasks.objects.filter(game_id=game_id, player_id=user_id).first()
    required_ix = list(task_archetypes.keys()).index(assigned_task.task_archetype)
    ixs = set([i for i in range(len(task_archetypes.items()))]) - set([required_ix])
    valid_ixs = random.sample(list(ixs), 3) + [required_ix]
    gui_quadrant_ixs = {0, 1, 2, 3}
    for i, (archetype, name_options) in enumerate(task_archetypes.items()):
        response["controllers"][archetype] = {}
        response["controllers"][archetype]["controller_name"] = TaskNameMappings.objects.values_list('task_name', flat=True).filter(game_id=game_id,player_id=user_id,task_archetype=archetype)[0]
        response["controllers"][archetype]["controller_goal"] = -1 if i != required_ix else assigned_task.goal
        response["controllers"][archetype]["number"] = set.pop(gui_quadrant_ixs) if i in valid_ixs else -1


def ritaank_func(cur, ass, goal):
    return 1
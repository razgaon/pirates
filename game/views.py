import random
import json
from datetime import datetime

from rest_framework.response import Response
from rest_framework.views import APIView

from .models import Game, Outstanding, TaskAssignment, TaskName
from rest_framework import viewsets, status
from rest_framework import permissions
from .serializers import TaskNameSerializer, GameSerializer, TaskAssignmentSerializer, OutstandingSerializer


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

task_goals = {"button-toggle": [15, 30],
                   "button-increment": [30, 45],
                   "button-LED-toggle": [5, 8],
                   "microphone-password": [6, 10],
                   "device-shake": [2, 4]}

NUM_PLAYERS = 2

class GamesViewSet(viewsets.ModelViewSet):
    """
    API endpoint that allows groups to be viewed or edited.
    """
    queryset = Game.objects.all()
    serializer_class = GameSerializer

class TaskNameMappingsViewSet(viewsets.ModelViewSet):
    """
    API endpoint that allows users to be viewed or edited.
    """
    queryset = TaskName.objects.all()
    serializer_class = TaskNameSerializer

class TaskCommunicationViewSet(viewsets.ModelViewSet):
    """
    API endpoint that allows groups to be viewed or edited.
    """
    queryset = TaskAssignment.objects.all()
    serializer_class = TaskAssignmentSerializer


class CurrentTasksViewSet(viewsets.ModelViewSet):
    """
    API endpoint that allows groups to be viewed or edited.
    """
    queryset = Outstanding.objects.all()
    serializer_class = OutstandingSerializer


class PlayerReady(APIView):
    """
    List all snippets, or create a new snippet.
    """

    def post(self, request, user_id, format=None):
        # First add the user.
        data = {'game_id': "game1", 'player_id': user_id}
        game_serializer = GameSerializer(data=data)
        if game_serializer.is_valid():
            game_serializer.save()
        # IF WE NOW HAVE THE COMPLETE GAME, WE'RE GOING TO DO A BUNCH OF STUFF
        ready_players = Game.objects.filter(game_id="game1")
        if len(ready_players) == NUM_PLAYERS:
            local_mappings = {}
            #first, lets generate name mappings
            for archetype, name_options in task_archetypes.items():
                rndm_options = random.sample(name_options, len(name_options))
                rndm_options = rndm_options[:NUM_PLAYERS]
                for i, player_obj in enumerate(ready_players):
                    data = {'game_id': 'game1', 
                            'player_id': player_obj.player_id, 
                            'task_archetype': archetype, 
                            'task_name': rndm_options[i]}
                    taskmapping_serializer = TaskNameMappingsSerializer(data=data)
                    taskmapping_serializer.save()
                    local_mappings[player_obj.player_id] = (archetype, rndm_options[i])
            #second, lets assign everyone tasks and communications
            possible_tasks = list(task_archetypes.keys())
            assigned_task_archetypes = [random.sample(possible_tasks, 1) for _ in range(NUM_PLAYERS)] 
            order = random.sample([i for i in range(NUM_PLAYERS)], NUM_PLAYERS)
            ts = datetime.now()
            for i, player_obj in enumerate(ready_players):
                #first, we assign the task to the person
                arch = local_mappings[player_obj.player_id][0]
                data = {'game_id': 'game1', 
                        'player_id': player_obj.player_id, 
                        'task_archetype': arch, 
                        'goal': random.randint(task_goals[arch][0], task_goals[arch][1]),
                        'finished':False,
                        'timestamp': ts}
                currenttasks_serializer = CurrentTasksSerializer(data=data)
                currenttasks_serializer.save()
                #now, lets establish communication expectations
                my_id = player_obj.player_id
                target_player_order = order[order.index(i)+1 % NUM_PLAYERS]
                target_player_id = ready_players[target_player_order].player_id
                task_archetype = local_mappings[target_player_id][0]
                task_name = local_mappings[target_player_id][1]
                text = task_texts[task_archetype].format(control=task_name)
                data = {'game_id': 'game1', 
                        'speaker_player_id': my_id, 
                        'listener_player_id': target_player_id, 
                        'task_archetype': task_archetype, 
                        'task_name': task_name,
                        'text': text}
                taskcomms_serializer = TaskCommunicationSerializer(data=data)
                taskcomms_serializer.save()
            #last, lets begin the game!
            Game.objects.filter(game_id="game1").update(score=0, timestamp=ts)
        
        return Response(f"user {user_id} is now ready", status=status.HTTP_200_OK)


class CheckStart(APIView):
    """
    List all snippets, or create a new snippet.
    """

    def get(self, request, game_id, user_id, format=None):
        assert game_id == "game1"
        # FIRST THING WE NEED TO DO: INSERT PLAYER INTO GAMESDB
        ready_players = Game.objects.filter(game_id=game_id).all()
        response = {}
        if len(ready_players) == NUM_PLAYERS:
            response["status"] = True
            response["controllers"] = {}
            response["text"] = TaskCommunication.objects.values_list('text', flat=True).filter(game_id="game1",speaker_player_id=user_id)[0]
            assigned_task = CurrentTasks.object.filter(game_id="game1", player_id=user_id)
            required_ix = list(task_archetypes.items()).index(assigned_task.task_archetype)
            ixs = set([i for i in range(len(task_archetypes.items()))]) - set([required_ix])
            valid_ixs = random.sample(list(ixs), 3) + [required_ix]
            gui_quadrant_ixs = {0, 1, 2, 3}
            for i, archetype, name_options in enumerate(task_archetypes.items()):
                response["controllers"][archetype] = {}
                response["controllers"][archetype]["controller_name"] = TaskNameMappings.objects.values_list('task_name', flat=True).filter(game_id="game1",player_id=user_id,task_archetype=archetype)[0]
                response["controllers"][archetype]["controller_goal"] = -1 if i != required_ix else assigned_task.goal
                response["controllers"][archetype]["number"] = set.pop(gui_quadrant_ixs) if i in valid_ixs else -1
            return Response(response)
        else:
            response["status"] = False
            return Response(json.dumps(response))

class TaskComplete(APIView):
    """
    Handles players logging a completed task
    """
    def post(self, request, user_id, format=None):
        CurrentTasks.objects.filter(game_id="game1", player_id=user_id).update(finished = True)
        time_assigned = CurrentTasks.objects.values_list('timestamp', flat=True).filter(game_id="game1",player_id=user_id)[0]
        goal = CurrentTasks.objects.values_list('goal', flat=True).filter(game_id="game1",player_id=user_id)[0]
        score = ritaank_func(datetime.now(), time_assigned, goals)
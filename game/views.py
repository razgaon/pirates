from datetime import datetime

from rest_framework.response import Response
from rest_framework.views import APIView

from .models import Game, Outstanding, TaskAssignment, TaskName
from rest_framework import viewsets, status
from rest_framework import permissions
from .serializers import TaskNameSerializer, GameSerializer, TaskAssignmentSerializer, OutstandingSerializer
import random

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

archetype_ix = {"button-toggle": 0,
                   "button-increment": 1,
                   "button-LED-toggle": 2,
                   "microphone-password": 3,
                   "device-shake": 4}

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
                data = {'game_id': 'game1', 
                        'player_id': player_obj.player_id, 
                        'task_archetype': local_mappings[player_obj.player_id][0], 
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
        
        return Response(f"user {user} is now ready", status=status.HTTP_200_OK)


class CheckStart(APIView):
    """
    List all snippets, or create a new snippet.
    """

    def post(self, request, game_id, user_id, format=None):
        assert game_id == "game1"
        # FIRST THING WE NEED TO DO: INSERT PLAYER INTO GAMESDB
        ready_players = Game.objects.filter(game_id=game_id).all()
        if len(ready_players) == NUM_PLAYERS:
            # if we're in here, we're ready to start the game... we can send stuff down now
            mappings = TaskName.objects.filter(game_id="game1", player_id=user_id).all()

            # get tasks that aren't assigned to the user (to be yelled)
            potential_tasks = TaskName.objects.exclude(player_id=user_id).all()  # TODO: missing game1 filter, add here
            # only assign tasks that haven't been assigned yet
            used_ids = {el.player_id for el in TaskAssignment.objects.all()}
            response = "READY;"
            for el in potential_tasks:
                if el.id not in used_ids:
                    # then we assign, and break
                    assignment = TaskAssignment(id=el.id, game_id="game1", player_id=user_id,
                                                task_archetype=el.task_archetype,
                                                task_name=el.task_name)
                    assignment.save()

                    task_text = task_texts[el.task_archetype].format(control=el.task_name)
                    response += f"{task_text};"
                    break

            for el in mappings:
                response += f"{el.task_archetype}:{el.task_name};"
            return Response(response)
        else:
            return Response("NOT READY")

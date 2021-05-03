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

NUM_PLAYERS = 2


class TaskNameViewSet(viewsets.ModelViewSet):
    """
    API endpoint that allows users to be viewed or edited.
    """
    queryset = TaskName.objects.all()
    serializer_class = TaskNameSerializer


class GameViewSet(viewsets.ModelViewSet):
    """
    API endpoint that allows groups to be viewed or edited.
    """
    queryset = Game.objects.all()
    serializer_class = GameSerializer


class TaskAssignmentViewSet(viewsets.ModelViewSet):
    """
    API endpoint that allows groups to be viewed or edited.
    """
    queryset = TaskAssignment.objects.all()
    serializer_class = TaskAssignmentSerializer


class OutstandingViewSet(viewsets.ModelViewSet):
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
        user = user_id
        data = {'game_id': "game1", 'player_id': user}
        game_serializer = GameSerializer(data=data)
        if game_serializer.is_valid():
            game_serializer.save()
        # now we also want to generate some task names for them
        for archetype, possible_choices in task_archetypes.items():
            current_in_use = TaskName.objects.filter(game_id="game1", task_archetype=archetype).all()
            available = set(possible_choices) - set(current_in_use)
            mapping = TaskName(game_id="game1", player_id=user, task_archetype=archetype,
                               task_name=random.sample(available, 1)[0])
            mapping.save()

        # check if game has started!!!
        ready_players = Game.objects.filter(game_id="game1")
        if len(ready_players) == NUM_PLAYERS:
            Game.objects.filter(game_id="game1").update(score=0, timestamp=datetime.now())

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

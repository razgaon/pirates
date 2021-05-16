from .models import *
from rest_framework import serializers


class GamesSerializer(serializers.ModelSerializer):
    class Meta:
        model = Games
        fields = ('game_id',
                  'player_id',
                  'score',
                  'round_num',
                  'num_players',
                  'esp_connected',
                  'timestamp')

    def create(self, validated_data):
        """
        Create and return a new `Snippet` instance, given the validated data.
        """
        return Games.objects.create(**validated_data)


class TaskNameMappingsSerializer(serializers.ModelSerializer):
    class Meta:
        model = TaskNameMappings
        fields = ('game_id',
                  'player_id',
                  'task_archetype',
                  'task_name')


class TaskCommunicationSerializer(serializers.ModelSerializer):
    class Meta:
        model = TaskCommunication
        fields = ('game_id',
                  'speaker_player_id',
                  'listener_player_id'
                  'task_archetype',
                  'task_name',
                  'text')


class CurrentTasksSerializer(serializers.ModelSerializer):
    class Meta:
        model = CurrentTasks
        fields = ['game_id',
                  'player_id',
                  'task_archetype',
                  'finished',
                  'timestamp']

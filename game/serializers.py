from .models import *
from rest_framework import serializers


class GameSerializer(serializers.ModelSerializer):
    class Meta:
        model = Game
        fields = ['game_id',
                  'player_id',
                  'score',
                  'timestamp', ]

    def create(self, validated_data):
        """
        Create and return a new `Snippet` instance, given the validated data.
        """
        return Game.objects.create(**validated_data)


class TaskNameSerializer(serializers.ModelSerializer):
    class Meta:
        model = TaskName
        fields = ('game_id',
                  'player_id',
                  'task_archetype',
                  'task_name')


class TaskAssignmentSerializer(serializers.ModelSerializer):
    class Meta:
        model = TaskAssignment
        fields = ('game_id',
                  'player_id',
                  'task_archetype',
                  'task_name')


class OutstandingSerializer(serializers.ModelSerializer):
    class Meta:
        model = Outstanding
        fields = ['game_id',
                  'player_id',
                  'task_archetype',
                  'timestamp']

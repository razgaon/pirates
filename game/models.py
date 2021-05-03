from django.db import models


# Create your models here.

class Games(models.Model):
    game_id = models.TextField(null=False)
    player_id = models.TextField(unique=True, null=False)
    score = models.IntegerField(default=None)
    timestamp = models.DateTimeField(auto_now=False, default=None)


class TaskNameMappings(models.Model):
    """
    Stores the mappings of task names to actual tasks
    """
    game_id = models.TextField(null=False)
    player_id = models.TextField(null=False)
    task_archetype = models.TextField(null=False)
    task_name = models.TextField(null=False)


class TaskCommunication(models.Model):
    """
    Stores which player is displaying which task
    """
    game_id = models.TextField(null=False)
    speaker_player_id = models.TextField(null=False)
    listener_player_id = models.TextField(null=False)
    task_archetype = models.TextField(null=False)
    task_name = models.TextField(null=False)
    text = models.TextField(null=False)


class CurrentTasks(models.Model):
    """
    Stores which tasks are still open to being completed
    """
    game_id = models.TextField(null=False)
    player_id = models.TextField(null=False)
    task_archetype = models.TextField(null=False)
    finished = models.BooleanField(default=False, null=False)
    timestamp = models.TextField(null=False)

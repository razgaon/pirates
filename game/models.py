from django.db import models


# Create your models here.

class Game(models.Model):
    game_id = models.TextField(null=False)
    player_id = models.TextField(unique=True, null=False)
    # ready = models.TextField(db.Boolean, null=False)
    score = models.IntegerField(default=None)
    timestamp = models.DateTimeField(auto_now=False, default=None)


class TaskName(models.Model):
    """
    Stores the mappings of task names to actual tasks
    """
    game_id = models.TextField(null=False)
    player_id = models.TextField(null=False)
    task_archetype = models.TextField(null=False)
    task_name = models.TextField(null=False)


class TaskAssignment(models.Model):
    """
    Stores which player is displaying which task
    """
    game_id = models.TextField(null=False)
    player_id = models.TextField(null=False)
    task_archetype = models.TextField(null=False)
    task_name = models.TextField(null=False)


class Outstanding(models.Model):
    """
    Stores which tasks are still open to being completed
    """
    game_id = models.TextField(null=False)
    player_id = models.TextField(null=False)
    task_archetype = models.TextField(null=False)
    timestamp = models.TextField(null=False)

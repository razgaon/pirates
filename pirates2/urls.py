"""pirates2 URL Configuration

The `urlpatterns` list routes URLs to views. For more information please see:
    https://docs.djangoproject.com/en/3.2/topics/http/urls/
Examples:
Function views
    1. Add an import:  from my_app import views
    2. Add a URL to urlpatterns:  path('', views.home, name='home')
Class-based views
    1. Add an import:  from other_app.views import Home
    2. Add a URL to urlpatterns:  path('', Home.as_view(), name='home')
Including another URLconf
    1. Import the include() function: from django.urls import include, path
    2. Add a URL to urlpatterns:  path('blog/', include('blog.urls'))
"""
from django.conf.urls import url
from django.contrib import admin
from django.urls import path, include
from rest_framework import routers
from game import views
from rest_framework import permissions
from drf_yasg.views import get_schema_view
from drf_yasg import openapi

router = routers.DefaultRouter()
router.register(r'db1', views.GamesViewSet)
router.register(r'db2', views.GamesViewSet)
router.register(r'db3', views.GamesViewSet)
router.register(r'db4', views.GamesViewSet)

schema_view = get_schema_view(
    openapi.Info(
        title="Pirates API",
        default_version='v1',
        description="This is the documentation of the pirates api.",
    ),
    public=True,
    permission_classes=(permissions.AllowAny,),
)

# Wire up our API using automatic URL routing.
# Additionally, we include login URLs for the browsable API.
urlpatterns = [
    path('', include(router.urls)),
    path('api-auth/', include('rest_framework.urls', namespace='rest_framework')),
    path('admin/', admin.site.urls),
    path('add_player/', views.AddPlayer.as_view()),
    path('user_ready/', views.AddPlayer.as_view()),
    path('check_start/', views.CheckStart.as_view()),
    path('task_complete/', views.TaskComplete.as_view()),
    path('get_new_round/', views.GetNewRound.as_view()),
    path('clear/', views.ClearGame.as_view()),
    path('score/', views.Score.as_view()),
    path('get_game_code/', views.GenerateGameCode.as_view()),
    path('create_game/', views.CreateGame.as_view()),
    path('start_game/', views.StartGame.as_view()),

    # Frontend
    path('get_games/', views.GetGames.as_view()),

    # Documentation
    url(r'^swagger(?P<format>\.json|\.yaml)$', schema_view.without_ui(cache_timeout=0), name='schema-json'),
    url(r'^swagger/$', schema_view.with_ui('swagger', cache_timeout=0), name='schema-swagger-ui'),
    url(r'^redoc/$', schema_view.with_ui('redoc', cache_timeout=0), name='schema-redoc'),
]

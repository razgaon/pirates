#include "Arduino.h"
#include "microphone.h"
#include <TFT_eSPI.h>

Microphone::Microphone() {
  success;
  controller_name;
  goal_word;
  quadrant;
  tft;
};

Microphone::Microphone(char* controller_name_inp, char* goal_inp, int q, TFT_eSPI t) {
  success = false;
  controller_name = controller_name_inp;
  goal_word = goal_inp;
  quadrant = q;
  tft = t;
};

Microphone::Microphone(TFT_eSPI t) {
  success = false;
  controller_name;
  goal_word;
  quadrant;
  tft = t;
};

char* Microphone::get_goal() {
  return goal_word;
};

void Microphone::set_name(char* controller_name_inp){
  controller_name = controller_name_inp;
}

void Microphone::set_goal(int goal_inp) {
  goal_word = "password";
}


bool Microphone::is_complete(char* transcript) {
  if (strstr(transcript, goal_word) != NULL)
  {
    success = true;
  }
  else {
    success = false;
  }
  return success;
};

char* Microphone::get_name() {
  return controller_name;
};

void Microphone::draw(bool s) {
  if (quadrant != -1) {
    int x = 0;
    int y = 0;
    if (quadrant == 0) {
      x = 0;
      y = 25;
    }
    else if (quadrant == 1) {
      x = 70;
      y = 25;
    }
    else if (quadrant == 2) {
      x = 0;
      y = 95;
    }
    else {
      x = 70;
      y = 95;
    }
    tft.setCursor(x, y - 10, 1);
    tft.println(controller_name);
    tft.setCursor(x, y + 10, 1);
    if (s) {
      tft.println("Success  ");
      tft.println("            ");
    }
    else {
      tft.println("Password:");
      tft.println(goal_word);
    }
  }
};

void Microphone::set_quadrant(int q) {
  quadrant = q;
};

#include "Arduino.h"
#include "button_class.h"
#include "button_toggler.h"
#include <TFT_eSPI.h>

Button_Toggler::Button_Toggler() {
  num_states = 2;
  prev_state = 0;
  state = 0;
  button;
  controller_name;
  goal;
  success = false;
  quadrant;
  tft;
};

Button_Toggler::Button_Toggler(Button button_inp, char* controller_name_inp, int goal_inp, int PWM_CHANNEL_R_inp, int PWM_CHANNEL_G_inp, int PWM_CHANNEL_B_inp, int num_states_inp) {
  num_states = num_states_inp;
  prev_state;
  state = 0;
  button = button_inp;
  controller_name = controller_name_inp;
  goal = goal_inp;
  success = false;
  quadrant;
  tft;
};


Button_Toggler::Button_Toggler(TFT_eSPI t) {
  num_states = 3;
  prev_state = 0;
  state = 0;
  button;
  controller_name;
  goal;
  success = false;
  quadrant;
  tft = t;
};


bool Button_Toggler::is_complete() {
  if (goal == state) {
    success = true;
    return true;
  }
  return false;
};

char* Button_Toggler::get_name() {
  return controller_name;
};

int Button_Toggler::get_state() {
  return state;
};

void Button_Toggler::set_name(char* controller_name_inp) {
  controller_name = controller_name_inp;
};

void Button_Toggler::set_button(Button button_inp) {
  button = button_inp;
};

void Button_Toggler::set_goal(int goal_inp) {
  goal = goal_inp;
};

void Button_Toggler::set_num_states(int num_states_inp) {
  num_states = num_states_inp;
};

bool Button_Toggler::is_updated() {
  if (prev_state != state) {
    return true;
  } else {
    return false;
  }
};

bool Button_Toggler::update() {
  int bv = button.update();
  bool incremented = false;
  if (bv == 2) { // long button press
    prev_state = state;
    state = (state + 1) % num_states;
    incremented = true;
  }
  return incremented;
};

void Button_Toggler::draw(bool s) {
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
    if (s) {
      tft.setCursor(x, y + 5, 1);
      tft.println("Success");
    }
    tft.setCursor(x + 20, y + 15, 1);
    tft.print("2");
    tft.setCursor(x + 20, y + 25, 1);
    tft.print("1");
    tft.setCursor(x + 20, y + 35, 1);
    tft.print("0");
    tft.fillRect(x + 27, y + 15, 10, 30, TFT_BLACK);
    tft.drawRect(x + 27, y + 15, 10, 30, TFT_WHITE);
    tft.drawLine(x + 28, y + 25, x + 36, y + 25, TFT_WHITE);
    tft.drawLine(x + 28, y + 35, x + 36, y + 35, TFT_WHITE);

    if (state == 2) {
      tft.fillRect(x + 27, y + 15, 10, 10, TFT_WHITE);
    }
    else if (state == 1) {
      tft.fillRect(x + 27, y + 25, 10, 10, TFT_WHITE);
    }
    else {
      tft.fillRect(x + 27, y + 35, 10, 10, TFT_WHITE);
    }
  }
}

void Button_Toggler::set_quadrant(int q) {
  quadrant = q;
}

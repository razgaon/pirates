#include "Arduino.h"
#include "button_class.h"
#include "button_toggler.h"


Button_Toggler::Button_Toggler() {
  num_states = 2;
  prev_state = 0;
  state = 0;
  button;
  controller_name;
  goal;
  success = false;
};

Button_Toggler::Button_Toggler(Button button_inp, char* controller_name_inp, int goal_inp, int PWM_CHANNEL_R_inp, int PWM_CHANNEL_G_inp, int PWM_CHANNEL_B_inp, int num_states_inp) {
  num_states = num_states_inp;
  prev_state;
  state = 0;
  button = button_inp;
  controller_name = controller_name_inp;
  goal = goal_inp;
  success = false;
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

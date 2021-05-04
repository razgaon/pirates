#include "Arduino.h"
#include "button_class.h"
#include "button_incrementer.h"

Button_Incrementer::Button_Incrementer() {
  prev_count = 0;
  count = 0;
  button;
  controller_name;
  goal;
  restart_timer = millis();
  RESET_TIME = 2000;
  success = false;
};

Button_Incrementer::Button_Incrementer(Button button_inp, char* controller_name_inp, int goal_inp, int PWM_CHANNEL_R_inp, int PWM_CHANNEL_G_inp, int PWM_CHANNEL_B_inp, int num_states_inp)
{
  prev_count = 0;
  count = 0;
  RESET_TIME = 2000;
  button = button_inp;
  controller_name = controller_name_inp;
  goal = goal_inp;
  success = false;
  restart_timer = millis();
};

bool Button_Incrementer::is_complete() {
  if (goal >= 0 && goal <= count) {
    success = true;
    return true;
  }
  return false;
};


bool Button_Incrementer::is_updated() {
  if (prev_count != count) {
    return true;
  } else {
    return false;
  }
};

char* Button_Incrementer::get_name() {
  return controller_name;
};

void Button_Incrementer::set_name(char* controller_name_inp) {
  controller_name = controller_name_inp;
};

void Button_Incrementer::set_button(Button button_inp) {
  button = button_inp;
};

void Button_Incrementer::set_goal(int goal_inp) {
  goal = goal_inp;
};

int Button_Incrementer::get_state() {
  return count;
};

bool Button_Incrementer::update() {
  int bv = button.update();
  bool incremented = false;
  if (bv != 0) { // button is pressed, either long or short
    restart_timer = millis();
    count++;
    incremented = true;
  } else if (millis() - restart_timer >= RESET_TIME) {
    if (count != 0) {
      incremented = true;
    }
    count = 0;
  }
  return incremented;
};

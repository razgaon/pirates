#include "Arduino.h"
#include "button_class.h"
#include "button_led.h"
#include <TFT_eSPI.h>

Button_LED::Button_LED() {
  num_states = 4;
  num_colors = 3;
  prev_state = 0;
  state = 0;
  button;
  controller_name;
  goal;
  color_channels[3];
  success = false;
  quadrant;
  tft;
};

Button_LED::Button_LED(Button button_inp, char* controller_name_inp, int goal_inp, int PWM_CHANNEL_R_inp, int PWM_CHANNEL_G_inp, int PWM_CHANNEL_B_inp, int num_states_inp) {
  num_states = 4;
  num_colors = 3;
  prev_state;
  state = 0;
  button = button_inp;
  controller_name = controller_name_inp;
  goal = goal_inp;
  color_channels[0] = PWM_CHANNEL_R_inp;
  color_channels[1] = PWM_CHANNEL_G_inp;
  color_channels[2] = PWM_CHANNEL_B_inp;
  success = false;
  quadrant;
  tft;
};

Button_LED::Button_LED(TFT_eSPI t) {
  num_states = 4;
  num_colors = 3;
  prev_state;
  state = 0;
  button;
  controller_name;
  goal;
  color_channels[3];
  success = false;
  quadrant;
  tft = t;
}

bool Button_LED::is_complete() {
  if (goal == state) {
    success = true;
    return true;
  }
  return false;
};

char* Button_LED::get_name() {
  return controller_name;
};

int Button_LED::get_state() {
  return state;
};

bool Button_LED::is_updated() {
  if (prev_state != state) {
    return true;
  } else {
    return false;
  }
};


void Button_LED::set_name(char* controller_name_inp) {
  controller_name = controller_name_inp;
};

void Button_LED::set_button(Button button_inp) {
  button = button_inp;
};

void Button_LED::set_goal(int goal_inp) {
  goal = goal_inp;
};

void Button_LED::set_num_states(int num_states_inp) {
  num_states = num_states_inp;
};

void Button_LED::set_color_channels(int PWM_CHANNEL_R_inp, int PWM_CHANNEL_G_inp, int PWM_CHANNEL_B_inp) {
  color_channels[0] = PWM_CHANNEL_R_inp;
  color_channels[1] = PWM_CHANNEL_G_inp;
  color_channels[2] = PWM_CHANNEL_B_inp;
};

bool Button_LED::update() {
  int bv = button.update();
  bool incremented = false;
  if (bv != 0) { // button is pressed, either long or short
    prev_state = state;
    state = (state + 1) % num_states;
    incremented = true;
    if (state != 0) {
      ledcWrite(color_channels[state - 1], 255);
      ledcWrite(color_channels[(state) % num_colors], 0);
      ledcWrite(color_channels[(state + 1) % num_colors], 0);
    } else {
      ledcWrite(color_channels[0], 0);
      ledcWrite(color_channels[1], 0);
      ledcWrite(color_channels[2], 0);
    }

  }
  return incremented;
};

void Button_LED::draw(bool s) {
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
      tft.fillRect(x + 15, y + 10, 25, 25, TFT_BLACK);
      tft.setCursor(x, y + 5, 1);
      tft.println("Success");
    }
    else {
      if (goal == 2) {
        tft.fillRect(x + 15, y + 10, 25, 25, TFT_RED);
      }
      else if (goal == 1) {
        tft.fillRect(x + 15, y + 10, 25, 25, TFT_GREEN);
      }
      else if (goal == 3) {
        tft.fillRect(x + 15, y + 10, 25, 25, TFT_BLUE);
      }
    }
  }
}

void Button_LED::set_quadrant(int q) {
  quadrant = q;
}

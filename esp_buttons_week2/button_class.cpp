#include "button_class.h"

Button::Button() {
  state_2_start_time;
  button_change_time;
  debounce_duration;
  long_press_duration;
  pin;
  flag;
  button_pressed;
  state;
};

Button::Button(const int p) {
  flag = 0;
  state = 0;
  pin = p;
  state_2_start_time = millis(); //init
  button_change_time = millis(); //init
  debounce_duration = 10;
  long_press_duration = 1000;
  button_pressed = 0;
};

void Button::read() {
  uint8_t button_state = digitalRead(pin);
  button_pressed = !button_state;
};

int Button::update() {
  read();
  flag = 0;
  if (state == 0) {
    if (button_pressed) {
      state = 1;
      button_change_time = millis();
    }
  } else if (state == 1) {
    if (button_pressed && millis() - button_change_time >= debounce_duration) {
      state = 2;
      state_2_start_time = millis();
    } else if (!button_pressed && millis() - button_change_time <= debounce_duration) {
      state = 0;
      button_change_time = millis();
    }
  } else if (state == 2) {
    if (button_pressed && millis() - state_2_start_time >= long_press_duration) {
      state = 3;
    } else if (!button_pressed) {
      state = 4;
      button_change_time = millis();
    }
  } else if (state == 3) {
    if (!button_pressed) {
      state = 4;
      button_change_time = millis();
    }
  } else if (state == 4) {
    if (button_pressed && millis() - state_2_start_time < long_press_duration) {
      state = 2;
      button_change_time = millis();
    } else  if (button_pressed && millis() - state_2_start_time >= long_press_duration) {
      state = 3;
      button_change_time = millis();
    } else  if (!button_pressed && millis() - button_change_time >= debounce_duration) {
      if (millis() - state_2_start_time >= long_press_duration) {
        flag = 2;
      } else {
        flag = 1;
      }
      state = 0;
    }
  }
  return flag;
};

#include "Arduino.h"
#include "shake.h"
#include <TFT_eSPI.h>

Shake::Shake() {
  prev_state;
  old_acc_mag;
  older_acc_mag;
  state;
  success;
  shakes;
  goal_shakes;
  controller_name;
  primary_timer;
  new_shake;
  quadrant;
  tft;
};

Shake::Shake(char* controller_name_inp, int goal_inp, int q, TFT_eSPI t) {
  prev_state;
  old_acc_mag = 0;
  older_acc_mag = 0;
  state = 0;
  success = false;
  controller_name = controller_name_inp;
  shakes = 0;
  goal_shakes = goal_inp;
  primary_timer = 0;
  new_shake = false;
  quadrant = q;
  tft = t;
};

Shake::Shake(TFT_eSPI t) {
  prev_state;
  old_acc_mag = 0;
  older_acc_mag = 0;
  state = 0;
  success = false;
  controller_name;
  shakes = 0;
  goal_shakes;
  primary_timer = 0;
  new_shake = false;
  quadrant;
  tft = t;
};

bool Shake::is_complete() {
  if (goal_shakes <= shakes && goal_shakes > 0) {
    success = true;
  }
  else {
    success = false;
  }
  return success;
};

char* Shake::get_name() {
  return controller_name;
};

int Shake::get_count() {
  return shakes;
};

bool Shake::update(float acc_mag) {
  float avg_acc_mag = 1.0 / 3.0 * (acc_mag + old_acc_mag + older_acc_mag);
  older_acc_mag = old_acc_mag;
  old_acc_mag = acc_mag;

  if (state == 0 && avg_acc_mag >= 14 / 9.81 && millis() - primary_timer >= 500) {
    prev_state = state;
    state = 1;
  }
  else if (state == 1 && avg_acc_mag < 10) {
    shakes++;
    prev_state = state;
    state = 0;
    primary_timer = millis();
    new_shake = true;
  }
  return new_shake;
};

void Shake::draw(bool s) {
  if (shakes == 0) {
    shakes = 1;
  }
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
      tft.println("Success");
    }
    else {
      tft.println(goal_shakes);
    }
  }
};

void Shake::set_quadrant(int q) {
  quadrant = q;
};

void Shake::set_name(char* controller_name_inp) {
  controller_name = controller_name_inp;
}

void Shake::set_goal(int goal_inp) {
  goal_shakes = goal_inp;
}

bool Shake::is_updated() {
  return (prev_state != state);
};

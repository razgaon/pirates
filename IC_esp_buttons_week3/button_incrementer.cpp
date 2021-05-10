#include "Arduino.h"
#include "button_class.h"
#include "button_incrementer.h"
#include <TFT_eSPI.h>


Button_Incrementer::Button_Incrementer() {
  prev_count = 0;
  count = 0;
  button;
  controller_name;
  goal;
  restart_timer = millis();
  RESET_TIME = 2000;
  success = false;
  tft;
  quadrant;
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
  tft;
  quadrant;
};

Button_Incrementer::Button_Incrementer(TFT_eSPI t) {
  prev_count = 0;
  count = 0;
  button;
  controller_name;
  goal;
  restart_timer = millis();
  RESET_TIME = 2000;
  success = false;
  tft = t;
  quadrant;
};

bool Button_Incrementer::is_complete() {
  if (goal < 0) {
    return false;
  } else if (count >= goal) {
    return true;
  } else {
    return false;
  }
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


void Button_Incrementer::set_quadrant(int q) {
  quadrant = q;
}

/*----------------------------------
  Draws on the screen for the specific quadrant

  Arguments:
  bool success true if the task was completed or false if it wasn't
*/
void Button_Incrementer::draw(bool s) {
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
    tft.fillRect(x, y, 50, 30, TFT_BLACK);
    int interval = 30 / goal;
    if (s) {
      tft.fillRect(x + 30, y + 40 - interval * count, 5, 5, TFT_BLACK);
      tft.println("Success    ");
    }
    else {
      Serial.println("Drawing the incrementer");
      tft.print("---------");
      tft.fillRect(x + 30, y + 40 - (interval * count - 1), 5, 5, TFT_BLACK);
      tft.fillRect(x + 30, y + 40 - (interval * count), 5, 5, TFT_ORANGE);
    }
  }
};

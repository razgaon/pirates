#ifndef button_incrementer_h
#define button_incrementer_h
#include "Arduino.h"
#include "button_class.h"
#include <TFT_eSPI.h>


class Button_Incrementer {
    uint8_t prev_count;
    uint8_t count;
    Button button;
    char* controller_name;
    int goal;
    int restart_timer;
    int RESET_TIME;
    bool success;
    int quadrant;
    TFT_eSPI tft;

  public:
    Button_Incrementer();

    /*----------------------------------
      This creates a Button Incrementer class.
      An incrementer stores a count of button presses (long or short), and resets if a new button press has not occured in 'restart_time'

      Arguments:
         Button button_inp: the button object to track presses from.
         char* controller_name_inp: the name of this particular controller (each controller has a different name, even if it is the same class)
         int goal_inp: the number of presses the button is trying to get to
    */
    Button_Incrementer(Button button_inp, char* controller_name_inp, int goal_inp, int PWM_CHANNEL_R_inp, int PWM_CHANNEL_G_inp, int PWM_CHANNEL_B_inp, int num_states_inp);


    Button_Incrementer(TFT_eSPI t);
    /*----------------------------------
      Checks to see whether the goal has been completed.
      If the goal has been completed then a POST request is sent to the server to update the backend.

      Returns:
         true if goal has been reached (at least once)
         false if goal still has not been reached
    */
    bool is_complete();

    /*----------------------------------
      Checks to see whether the goal has been updated.
      If the goal has been completed then a POST request is sent to the server to update the backend.

      Returns:
         true if goal has been reached (at least once)
         false if goal still has not been reached
    */
    bool is_updated();

    /*----------------------------------
      Gets the current count of the Incrementer

      Returns:
        int that represents the current count
    */
    int get_state();


    /*----------------------------------
      Gets the name of this specific controller

      Returns:
         char* the name of the controller
    */
    char* get_name();

    /*----------------------------------
      Updates the counts by 1 if the button is pressed (long or short).
      If RESET_TIME passes and no button has been pressed, it resets counts to be 0

      Returns:
         true if counts changed from the previous counts
         false if not
    */
    bool update();


    void set_name(char* controller_name_inp);

    void set_button(Button button_inp);

    void set_goal(int goal_inp);

    void set_quadrant(int q);
    
    /*----------------------------------
      Draws on the screen for the specific quadrant

      Arguments:
      bool success true if the task was completed or false if it wasn't
    */
    void draw(bool s);

};

#endif

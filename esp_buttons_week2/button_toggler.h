#ifndef button_toggler_h
#define button_toggler_h
#include "Arduino.h"
#include "button_class.h"
#include "GenericController.h"


class Button_Toggler {
    int prev_state;
    int state;
    Button button;
    char* controller_name;
    int goal;
    int num_states;
    bool success;
  public:
    Button_Toggler();

    /*----------------------------------
      This creates a Button Toggler class.
      This class toggles through the a set of states with each long button press.
      It starts at state 0 and increases and then wraps back around.

      Arguments:
         Button button_inp: the button object to track presses from.
         char* controller_name_inp: the name of this particular controller (each controller has a different name, even if it is the same class)
         int goal_inp: the target state of the Toggler (0 <= goal_inp <= num_states_inp -1)
         int num_states_inp: the number of states that this controller can toggle through
    */
    Button_Toggler(Button button_inp, char* controller_name_inp, int goal_inp, int PWM_CHANNEL_R_inp, int PWM_CHANNEL_G_inp, int PWM_CHANNEL_B_inp, int num_states_inp);

    /*----------------------------------
      Checks to see whether the goal has been completed.
      If the goal has been completed then a POST request is sent to the server to update the backend.

      Returns:
        true if goal has been reached (at least once)
        false if goal still has not been reached
    */
    bool is_complete();

    /*----------------------------------
      Gets the current state of the toggler

      Returns:
         int that represents the current state
    */
    int get_state();


    /*----------------------------------
      Gets the name of this specific controller

      Returns:
        char* the name of the controller
    */
    char* get_name();

    /*----------------------------------
      Updates the state by 1 if the button does a long press
      Wraps around the top, so if it goes over num_states it mods around.

      Returns:
         true if state has changed from the previous state
         false if not
    */
    bool update();

    bool is_updated();

    void set_name(char* controller_name_inp);

    void set_button(Button button_inp);

    void set_goal(int goal_inp);

    void set_num_states(int num_states_inp);

};

#endif

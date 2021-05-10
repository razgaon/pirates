#ifndef button_led_h
#define button_led_h
#include "Arduino.h"
#include "button_class.h"
#include <TFT_eSPI.h>

class Button_LED {
    int prev_state;
    int state;
    Button button;
    char* controller_name;
    int goal;
    int num_states;
    int color_channels[3];
    int num_colors;
    bool success;
    int quadrant;
    TFT_eSPI tft;

  public:
    Button_LED();

    /*----------------------------------
      This creates a Button LED class.
      This class toggles through the LED colors when the button is pressed (long or short)
      It starts off and then toggles through RGB and back to off.

      Arguments:
         Button button_inp: the button object to track presses from.
         char* controller_name_inp: the name of this particular controller (each controller has a different name, even if it is the same class)
         int goal_inp: the target state of the LED (0:OFF, 1:R, 2:G, 3:B)
         int PWM_CHANNEL_R_inp, int PWM_CHANNEL_G_inp, int PWM_CHANNEL_B_inp: The channels of the PWM pins attached to the LED
    */
    Button_LED(Button button_inp, char* controller_name_inp, int goal_inp, int PWM_CHANNEL_R_inp, int PWM_CHANNEL_G_inp, int PWM_CHANNEL_B_inp, int num_states_inp);


    Button_LED(TFT_eSPI t);

    /*----------------------------------
      Checks to see whether the goal has been completed.
      If the goal has been completed then a POST request is sent to the server to update the backend.

      Returns:
        true if goal has been reached (at least once)
        false if goal still has not been reached
    */
    bool is_complete();

    /*----------------------------------
      Gets the current state of the LED as a number where 0 represents off, 1 represents red, 2 represents green and 3 represents blue

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
      Increments the state of the system by 1 and therefore toggles between LED colors (and sets LED)

      Returns:
         true if color of LED has changed from the previous
         false if not
    */
    bool update();


    bool is_updated();

    void set_name(char* controller_name_inp);

    void set_button(Button button_inp);

    void set_goal(int goal_inp);

    void set_num_states(int num_states_inp);

    void set_color_channels(int PWM_CHANNEL_R_inp, int PWM_CHANNEL_G_inp, int PWM_CHANNEL_B_inp);

    /*----------------------------------
      Draws on the screen for the specific quadrant

      Arguments:
      bool success true if the task was completed or false if it wasn't
    */
    void draw(bool s);

    void set_quadrant(int q);

};

#endif

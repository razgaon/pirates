#ifndef shake_h
#define shake_h
#include "Arduino.h"
#include "shake.h"
#include <TFT_eSPI.h>


class Shake {
    uint8_t prev_state;
    float old_acc_mag;  //previous acc mag
    float older_acc_mag;  //previous prevoius acc mag
    uint8_t state;  //system state for shake counting
    bool success;
    int shakes;
    int goal_shakes;
    uint32_t primary_timer;
    bool new_shake;
    char* controller_name;
    int quadrant;
    TFT_eSPI tft;

  public:
    Shake();

    /*----------------------------------
      This creates a Shake class.
      A shaker stores a count of shakes that a controller does

      Arguments:
         char* controller_name_inp: the name of this particular controller (each controller has a different name, even if it is the same class)
         int goal_inp: the number of shakes the controller is trying to get to
    */
    Shake(char* controller_name_inp, int goal_inp, int q, TFT_eSPI t);

    Shake(TFT_eSPI t);


    /*----------------------------------
      Checks to see whether the goal has been completed.
      If the goal has been completed then a POST request is sent to the server to update the backend.

      Returns:
         true if goal has been reached (at least once)
         false if goal still has not been reached
    */
    bool is_complete();

    /*----------------------------------
      Gets the current number of shakes

      Returns:
        int that represents the current shakes
    */
    int get_count();


    /*----------------------------------
      Gets the name of this specific controller

      Returns:
        char* the name of the controller
    */
    char* get_name();

    /*----------------------------------
      Updates the counts by 1 if the button is pressed (long or short).
      If RESET_TIME passes and no button has been pressed, it resets counts to be 0

      Arguments:
         acc_mag (float) the magnitude of the acceleration that was just read by the IMU
      Returns:
         true if counts changed from the previous counts
         false if not
    */
    bool update(float acc_mag);

    /*----------------------------------
      Draws on the screen for the specific quadrant

      Arguments:
      bool success true if the task was completed or false if it wasn't
    */
    void draw(bool s);

    void set_quadrant(int q);

    void set_name(char* controller_name_inp);
    
    void set_goal(int goal_inp);

    bool is_updated();

};

#endif

#ifndef microphone_h
#define microphone_h
#include "Arduino.h"
#include "microphone.h"
#include <TFT_eSPI.h>

class Microphone {
    bool success;
    char* goal_word;
    char* controller_name;
    int quadrant;
    TFT_eSPI tft;

  public:
    Microphone();

    /*----------------------------------
      This creates a Microphone class.
      A microphone stores checks if the word has been spoken

      Arguments:
         char* controller_name_inp: the name of this particular controller (each controller has a different name, even if it is the same class)
         char* goal_inp: the word the controller is trying to get to
    */
    Microphone(char* controller_name_inp, char* goal_inp, int q, TFT_eSPI t);

    Microphone(TFT_eSPI t);

    /*----------------------------------
      Gets the goal word

      Returns:
        char that represents the goal word
    */
    char* get_goal();

    void set_name(char* controller_name_inp);
    
    void set_goal(int goal_inp);

    /*----------------------------------
      Checks to see whether the goal has been completed.
      If the goal has been completed then a POST request is sent to the server to update the backend.

      Returns:
         true if goal has been reached (at least once)
         false if goal still has not been reached
    */

    bool is_complete(char* transcript);

    /*----------------------------------
      Gets the name of this specific controller

      Returns:
        char* the name of the controller
    */
    char* get_name();

    /*----------------------------------
      Draws on the screen for the specific quadrant

      Arguments:
      bool success true if the task was completed or false if it wasn't
    */
    void draw(bool s);

    void set_quadrant(int q);
};

#endif

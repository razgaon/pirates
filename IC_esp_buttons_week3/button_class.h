#ifndef button_class_h
#define button_class_h
#include "Arduino.h"

class Button {
    uint32_t state_2_start_time;
    uint32_t button_change_time;
    uint32_t debounce_duration;
    uint32_t long_press_duration;
    uint8_t pin;
    uint8_t flag;
    bool button_pressed;
    uint8_t state;
  public:
    Button();

    /*----------------------------------
      This creates a button class. A button uses a specific pin, and represents different length presses as an int.

      Arguments:
         const int p: the pin number the button is assigned to
    */
    Button(const int p);

    /*----------------------------------
      Reads the input from the pin and assigns button_pressed to 1 if the button was pressed, and 0 otherwise.
    */
    void read();

    /*----------------------------------
      Tells the client if a button pressed has occured (an update).

      Returns:
        0 if no update has occured
        1 if a short press has occured
        2 if a long press has occured
    */
    int update();
};

#endif

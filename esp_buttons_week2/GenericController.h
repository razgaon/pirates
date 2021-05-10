#ifndef GenericController_h
#define GenericController_h
class GenericController {
  
  public:
    GenericController();

    GenericController(Button button_inp, char* controller_name_inp, int goal_inp, int PWM_CHANNEL_R_inp, int PWM_CHANNEL_G_inp, int PWM_CHANNEL_B_inp, int num_states_inp);
//
//    virtual bool is_complete();
//
//    virtual bool is_updated();
//
//    virtual int get_state();
//
//    virtual char* get_name();
//
//    virtual bool update();

//    virtual void draw();
};

#endif

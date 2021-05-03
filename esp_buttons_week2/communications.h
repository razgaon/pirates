//#ifndef communications_h
//#define communications_h
//#include "Arduino.h"
//
//class Communications {
//    int RESPONSE_TIMEOUT;
//    uint16_t OUT_BUFFER_SIZE;
//    char* response;
//
//    int game_id;
//    char* player_name;
//
//  public:
//    Communications();
//
//    /*----------------------------------
//      Initalizes a new Communications object.
//      The object is used for each player to communicate effeciently with the backend.
//
//      Arguments:
//        const int RESPONSE_TIMEOUT: ms to wait for response from host
//        const uint16_t OUT_BUFFER_SIZE: size of buffer to hold HTTP response
//        char old_response: char array buffer to hold HTTP request
//        char response: char array buffer to hold HTTP request
//        int game_id: the game_id that the player wishes to join
//        char* player_name: the name of the player joining the game
//    */
//  Communications(int game_id_inp, char* player_name_inp);
//
//
//    /*----------------------------------
//      Sends a POST request to 'players_ready.py' indicating that the client is ready to play the game.
//
//      Arguments:
//        int game_id: the game_id that the player wishes to join
//        char* player_name: the name of the player joining the game
//        
//      Response Format:
//        JSON
//    */
//    void post_ready_to_play();
//
//
//    /*----------------------------------
//      Sends a GET request to 'get_new_round.py' which responds to the player with new controllers, their goals
//      (-1 for non-used, and real goal for yes used) and your task for this round (to shout at others).
//
//      This will be sent periodically to the backend throughout the game as well, and the response will indicate whether or not the round is ongoing.
//      Only when a round ends will the server respond with new controllers and tasks.
//
//      Response Format:
//        JSON
//    */
//    char* get_new_round();
//
//    
//    /*----------------------------------
//      Sends a POST request to 'task_logger.py' updating the server that a task has been completed
//
//      Arguments:
//        char* controller_name: the name of the controller that has completed a task
//        
//      Response Format:
//        JSON
//    */
//    void post_completed_task(char* controller_name);
//
//  private:
//    /*----------------------------------
//      char_append Function:
//      Arguments:
//         char* buff: pointer to character array which we will append a
//         char c:
//         uint16_t buff_size: size of buffer buff
//
//      Return value:
//         boolean: True if character appended, False if not appended (indicating buffer full)
//    */
//    uint8_t char_append(char* buff, char c, uint16_t buff_size);
//
//    /*----------------------------------
//       do_http_request Function:
//       Arguments:
//          char* host: null-terminated char-array containing host to connect to
//          char* request: null-terminated char-arry containing properly formatted HTTP request
//          char* response: char-array used as output for function to contain response
//          uint16_t response_size: size of response buffer (in bytes)
//          uint16_t response_timeout: duration we'll wait (in ms) for a response from server
//          uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
//       Return value:
//          void (none)
//    */
//    void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial);
//};
//
//#endif

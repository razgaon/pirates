//#include "Arduino.h"
//#include "communications.h"
//#include <WiFi.h> //Connect to WiFi Network
//
//
//Communications::Communications() {
//  RESPONSE_TIMEOUT;
//  OUT_BUFFER_SIZE;
//  //  response;
//
//  game_id;
//  player_name;
//}
//
//Communications::Communications(int game_id_inp, char* player_name_inp) {
//  RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
//  OUT_BUFFER_SIZE = 2000; //size of buffer to hold HTTP response
//  //  response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
//
//  game_id = game_id_inp;
//  player_name = player_name_inp;
//}
//
//void Communications::post_ready_to_play() {
//  char request_buffer[OUT_BUFFER_SIZE];
//  char response[OUT_BUFFER_SIZE];
//  char body[100]; //for body
//  sprintf(body, "player=%s", player_name); //generate body
//  int body_len = strlen(body); //calculate body length (for header reporting)
//  sprintf(request_buffer, "POST http://608dev-2.net/sandbox/sc/team03/players_ready.py HTTP/1.1\r\n"); // TODO!!!!!
//  strcat(request_buffer, "Host: 608dev-2.net\r\n");
//  strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
//  sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
//  strcat(request_buffer, "\r\n"); //new line from header to body
//  strcat(request_buffer, body); //body
//  strcat(request_buffer, "\r\n"); //new line
//  Serial.println(request_buffer);
//  do_http_request("608dev-2.net", request_buffer, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
//  Serial.println(response); //viewable in Serial Terminal
//}
//
//char* Communications::get_new_round() {
//  char request_buffer[OUT_BUFFER_SIZE];
//  char response[OUT_BUFFER_SIZE];
//
//  sprintf(request_buffer, "GET http://608dev-2.net/sandbox/sc/team03/get_new_round.py HTTP/1.1\r\n"); // TODO!!!!!
//  strcat(request_buffer, "Host: 608dev-2.net\r\n");
//  strcat(request_buffer, "\r\n"); //new line
//  Serial.println(request_buffer);
//  do_http_request("608dev-2.net", request_buffer, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
//  Serial.println(response); //viewable in Serial Terminal
//  return response;
//}
//
//void Communications::post_completed_task(char* controller_name) {
//  char request_buffer[OUT_BUFFER_SIZE];
//  char response[OUT_BUFFER_SIZE];
//
//  char body[100]; //for body
//  sprintf(body, "controller_name=%s", controller_name); //generate body
//  int body_len = strlen(body); //calculate body length (for header reporting)
//  sprintf(request_buffer, "POST http://608dev-2.net/sandbox/sc/team03/task_logger.py HTTP/1.1\r\n"); // TODO!!!!!
//  strcat(request_buffer, "Host: 608dev-2.net\r\n");
//  strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
//  sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
//  strcat(request_buffer, "\r\n"); //new line from header to body
//  strcat(request_buffer, body); //body
//  strcat(request_buffer, "\r\n"); //new line
//  Serial.println(request_buffer);
//
//  do_http_request("608dev-2.net", request_buffer, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
//  Serial.println(response); //viewable in Serial Terminal
//}
//
//uint8_t Communications::char_append(char* buff, char c, uint16_t buff_size) {
//  int len = strlen(buff);
//  if (len > buff_size) return false;
//  buff[len] = c;
//  buff[len + 1] = '\0';
//  return true;
//}
//void Communications::do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial) {
//  WiFiClient client; //instantiate a client object
//  if (client.connect(host, 80)) { //try to connect to host on port 80
//    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
//    client.print(request);
//    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
//    uint32_t count = millis();
//    while (client.connected()) { //while we remain connected read out data coming back
//      client.readBytesUntil('\n', response, response_size);
//      if (serial) Serial.println(response);
//      if (strcmp(response, "\r") == 0) { //found a blank line!
//        break;
//      }
//      memset(response, 0, response_size);
//      if (millis() - count > response_timeout) break;
//    }
//    memset(response, 0, response_size);
//    count = millis();
//    while (client.available()) { //read out remaining text (body of response)
//      char_append(response, client.read(), OUT_BUFFER_SIZE);
//    }
//    if (serial) Serial.println(response);
//    client.stop();
//    if (serial) Serial.println("-----------");
//  } else {
//    if (serial) Serial.println("connection failed :/");
//    if (serial) Serial.println("wait 0.5 sec...");
//    client.stop();
//  }
//}

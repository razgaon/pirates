#include <WiFi.h> //Connect to WiFi Network
#include <SPI.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <mpu6050_esp32.h>
#include <math.h>
#include <string.h>
#include "button_incrementer.h"
#include "button_toggler.h"
#include "button_led.h"
#include "button_class.h"
//#include "communications.h"
//#include "GenericController.h"

// ESP constants
TFT_eSPI tft = TFT_eSPI();
const int SCREEN_HEIGHT = 160;
const int SCREEN_WIDTH = 128;
const int PIN1 = 0; //incrementer
const int PIN2 = 19; //toggler
const int PIN3 = 5; //button-led
const int PIN4 = 13;

// LED constants
const int R_PIN = 27;
const int G_PIN = 33;
const int B_PIN = 32;
const uint32_t PWM_CHANNEL_R = 0; //hardware pwm channel
const uint32_t PWM_CHANNEL_G = 1; //hardware pwm channel
const uint32_t PWM_CHANNEL_B = 2; //hardware pwm channel

const int LOOP_PERIOD = 40;

// Network constants
char network[] = "walhallaP";
char password[] = "modernvase639";

const int RESPONSE_TIMEOUT = 6000;     //ms to wait for response from host
const uint16_t OUT_BUFFER_SIZE = 1500; //size of buffer to hold HTTP response
char request[OUT_BUFFER_SIZE];
char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request

// other
uint32_t primary_timer;
uint32_t LOOP_INTERVAL = 5000;
bool prev_incrementer;
bool prev_toggler;
bool prev_led;

bool initialize;

Button button1(PIN1); //button object!
Button button2(PIN2); //button object!
Button button3(PIN3); //button object!
Button button4(PIN4); //button object!

// Each player fills this in before run (in future will make user input)
char *game_id = "game1";
char *player_name = "ritaank";
int round_num = 0;
//char* json_response;

Button_Incrementer incrementer = Button_Incrementer();
Button_Toggler toggler = Button_Toggler();
Button_LED button_led = Button_LED();

bool game_status;
char *task_display;
JsonObject controllers_button_incrementer;
char *controllers_button_incrementer_controller_name;
int controllers_button_incrementer_controller_goal;
int controllers_button_incrementer_number;
JsonObject controllers_button_toggler;
char *controllers_button_toggler_controller_name;
int controllers_button_toggler_controller_goal;
int controllers_button_toggler_number;
JsonObject controllers_button_led;
char *controllers_button_led_controller_name;
int controllers_button_led_controller_goal;
int controllers_button_led_number;

bool incrementer_updated = false;
bool button_toggler_updated = false;
bool button_led_updated = false;
bool round_started = false;
bool game_over = false;

/*----------------------------------
  char_append Function:
  Arguments:
     char* buff: pointer to character array which we will append a
     char c:
     uint16_t buff_size: size of buffer buff

  Return value:
     boolean: True if character appended, False if not appended (indicating buffer full)
*/
uint8_t char_append(char *buff, char c, uint16_t buff_size)
{
  int len = strlen(buff);
  if (len > buff_size)
    return false;
  buff[len] = c;
  buff[len + 1] = '\0';
  return true;
}

/*----------------------------------
   do_http_request Function:
   Arguments:
      char* host: null-terminated char-array containing host to connect to
      char* request: null-terminated char-arry containing properly formatted HTTP request
      char* response: char-array used as output for function to contain response
      uint16_t response_size: size of response buffer (in bytes)
      uint16_t response_timeout: duration we'll wait (in ms) for a response from server
      uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
   Return value:
      void (none)
*/
void do_http_request(char *host, char *request, char *response, uint16_t response_size, uint16_t response_timeout, uint8_t serial)
{
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80))
  { //try to connect to host on port 80
    if (serial)
      Serial.print(request); //Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected())
    { //while we remain connected read out data coming back
      client.readBytesUntil('\n', response, response_size);
      if (serial)
        Serial.println(response);
      if (strcmp(response, "\r") == 0)
      { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis() - count > response_timeout)
        break;
    }
    memset(response, 0, response_size);
    count = millis();
    while (client.available())
    { //read out remaining text (body of response)
      char_append(response, client.read(), OUT_BUFFER_SIZE);
    }
    if (serial)
      Serial.println(response);
    client.stop();
    if (serial)
      Serial.println("-----------");
  }
  else
  {
    if (serial)
      Serial.println("connection failed :/");
    if (serial)
      Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

void post_ready_to_play(char *game_id, char *player_name, char *request_buffer, char *response, uint16_t response_size, uint16_t response_timeout)
{
  char body[100];                                                                           //for body
  sprintf(body, "game_id=%s&user_id=%s", game_id, player_name);                             //generate body
  int body_len = strlen(body);                                                              //calculate body length (for header reporting)
  sprintf(request_buffer, "POST https://shipgroups.herokuapp.com/user_ready/ HTTP/1.1\r\n"); // TODO!!!!!
  strcat(request_buffer, "Host: shipgroups.herokuapp.com\r\n");
  strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
  strcat(request_buffer, "\r\n");                                                       //new line from header to body
  strcat(request_buffer, body);                                                         //body
  strcat(request_buffer, "\r\n");                                                       //new line
  Serial.println(request_buffer);
  do_http_request("shipgroups.herokuapp.com", request_buffer, response, response_size, response_timeout, true);
  Serial.println(response); //viewable in Serial Terminal
}

char *check_start(char *game_id, char *player_name, char *request_buffer, char *response, uint16_t response_size, uint16_t response_timeout)
{
  sprintf(request_buffer, "GET https://shipgroups.herokuapp.com/check_start/?game_id=%s&user_id=%s HTTP/1.1\r\n", game_id, player_name); // TODO!!!!!
  strcat(request_buffer, "Host: shipgroups.herokuapp.com\r\n");
  strcat(request_buffer, "\r\n"); //new line
  Serial.println(request_buffer);
  do_http_request("shipgroups.herokuapp.com", request_buffer, response, response_size, response_timeout, true);
  Serial.println(response); //viewable in Serial Terminal
  return response;
}

char *get_new_round(char *game_id, char *player_name, int round_num, char *request_buffer, char *response, uint16_t response_size, uint16_t response_timeout)
{
  sprintf(request_buffer, "GET https://shipgroups.herokuapp.com/get_new_round/?game_id=%s&user_id=%s&round_num=%d HTTP/1.1\r\n", game_id, player_name, round_num); // TODO!!!!!
  strcat(request_buffer, "Host: shipgroups.herokuapp.com\r\n");
  strcat(request_buffer, "\r\n"); //new line
  Serial.println(request_buffer);
  do_http_request("shipgroups.herokuapp.com", request_buffer, response, response_size, response_timeout, true);
  Serial.println(response); //viewable in Serial Terminal
  return response;
}

void post_completed_task(char *game_id, char *player_name, char *request_buffer, char *response, uint16_t response_size, uint16_t response_timeout)
{
  char body[100];                                                                              //for body
  sprintf(body, "game_id=%s&user_id=%s", game_id, player_name);                                 //generate body
  int body_len = strlen(body);                                                                 //calculate body length (for header reporting)
  sprintf(request_buffer, "POST https://shipgroups.herokuapp.com/task_complete/ HTTP/1.1\r\n"); // TODO!!!!!
  strcat(request_buffer, "Host: shipgroups.herokuapp.com\r\n");
  strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
  strcat(request_buffer, "\r\n");                                                       //new line from header to body
  strcat(request_buffer, body);                                                         //body
  strcat(request_buffer, "\r\n");                                                       //new line
  Serial.println(request_buffer);

  do_http_request("shipgroups.herokuapp.com", request_buffer, response, response_size, response_timeout, true);
  Serial.println(response); //viewable in Serial Terminal
}


void get_clear(char *request_buffer, char *response, uint16_t response_size, uint16_t response_timeout)
{
  sprintf(request_buffer, "GET https://shipgroups.herokuapp.com/clear/ HTTP/1.1\r\n"); // TODO!!!!!
  strcat(request_buffer, "Host: shipgroups.herokuapp.com\r\n");
  strcat(request_buffer, "\r\n"); //new line
  Serial.println(request_buffer);
  do_http_request("shipgroups.herokuapp.com", request_buffer, response, response_size, response_timeout, true);
  Serial.println(response); //viewable in Serial Terminal
}

void setup()
{
  Serial.begin(115200);          //for debugging if needed.
  WiFi.begin(network, password); //attempt to connect to wifi
  uint8_t count = 0;             //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 12)
  {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected())
  { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                  WiFi.localIP()[1], WiFi.localIP()[0],
                  WiFi.macAddress().c_str(), WiFi.SSID().c_str());
    delay(500);
  }
  else
  { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  // LED setup

  ledcSetup(PWM_CHANNEL_R, 120, 8);    //create pwm channel, @50 Hz, with 8 bits of precision
  ledcAttachPin(R_PIN, PWM_CHANNEL_R); //link pwm channel to IO pin r
  ledcSetup(PWM_CHANNEL_G, 120, 8);    //create pwm channel, @50 Hz, with 8 bits of precision
  ledcAttachPin(G_PIN, PWM_CHANNEL_G); //link pwm channel to IO pin g
  ledcSetup(PWM_CHANNEL_B, 120, 8);    //create pwm channel, @50 Hz, with 8 bits of precision
  ledcAttachPin(B_PIN, PWM_CHANNEL_B); //link pwm channel to IO pin b

  // display setup
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK); //set color of font to green foreground, black background
  tft.setCursor(0, 0, 1);
  pinMode(PIN1, INPUT_PULLUP);
  pinMode(PIN2, INPUT_PULLUP);
  pinMode(PIN3, INPUT_PULLUP);
  pinMode(PIN4, INPUT_PULLUP);

  primary_timer = millis();
  initialize = false;

  // TEMPORARY: clear the db before initializing this player
  get_clear(request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT);

  // indicate ready to play
  post_ready_to_play(game_id, player_name, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT);

  //    bool game_status = false;
  // send init GET to server to retrieve first controllers
  char *json_response = check_start(game_id, player_name, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT);

  //  parse JSON here
  StaticJsonDocument<2000> doc;

  DeserializationError error = deserializeJson(doc, json_response);

  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  game_status = doc["status"]; // true

  if (game_status)
  {
    round_started = true;
    const char* task = doc["task"]; // "Increment the scadoodle to 10"
    task_display = (char*)task;
    JsonObject controllers = doc["controllers"];

    controllers_button_incrementer = controllers["button_incrementer"];
    controllers_button_incrementer_controller_name = strdup(controllers_button_incrementer["controller_name"]);
    controllers_button_incrementer_controller_goal = controllers_button_incrementer["controller_goal"];
    controllers_button_incrementer_number = controllers_button_incrementer["number"]; // 1

    incrementer.set_button(button1);
    incrementer.set_name(controllers_button_incrementer_controller_name);
    incrementer.set_goal(controllers_button_incrementer_controller_goal);

    controllers_button_toggler = controllers["button_toggler"];
    controllers_button_toggler_controller_name = strdup(controllers_button_toggler["controller_name"]);
    controllers_button_toggler_controller_goal = controllers_button_toggler["controller_goal"]; // -1
    controllers_button_toggler_number = controllers_button_incrementer["number"];               // 1

    toggler.set_button(button2);
    toggler.set_name(controllers_button_toggler_controller_name);
    toggler.set_goal(controllers_button_toggler_controller_goal);
    toggler.set_num_states(2);

    controllers_button_led = controllers["button_led"];
    controllers_button_led_controller_name = strdup(controllers_button_led["controller_name"]);
    controllers_button_led_controller_goal = controllers_button_led["controller_goal"]; // -1
    controllers_button_led_number = controllers_button_incrementer["number"];           //

    button_led.set_button(button3);
    button_led.set_name(controllers_button_led_controller_name);
    button_led.set_goal(controllers_button_led_controller_goal);
    button_led.set_color_channels(PWM_CHANNEL_R, PWM_CHANNEL_G, PWM_CHANNEL_B);

    //    display
    tft.setCursor(0, 0, 1);
    tft.fillScreen(TFT_BLACK);
    tft.println(task);
    tft.println("---------------------");
    tft.println(incrementer.get_name());

    if (incrementer.is_complete())
    {
      tft.println("Success");
      prev_incrementer = true;
    }
    else
    {
      tft.println(incrementer.get_state());
    }

    tft.println("---------------------");

    tft.println(toggler.get_name());
    if (toggler.is_complete())
    {
      tft.println("Success");
      prev_toggler = true;
    }
    else
    {
      tft.println(toggler.get_state());
    }

    tft.println("---------------------");

    tft.println(button_led.get_name());
    if (button_led.is_complete())
    {
      tft.println("Success");
      prev_led = true;
    }
    else
    {
      tft.println(button_led.get_state());
    }
  }

  while (!game_status)
  {
    if (millis() - primary_timer >= LOOP_INTERVAL)
    {
      primary_timer = millis();
      // send init GET to server to retrieve first controllers
      char *json_response = check_start(game_id, player_name, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT);
  
      //  parse JSON here
      StaticJsonDocument<2000> doc;
  
      DeserializationError error = deserializeJson(doc, json_response);
  
      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
  
      game_status = doc["status"]; // true
      if (game_status)
      {
        const char* task = doc["task"]; // "Increment the scadoodle to 10"
        JsonObject controllers = doc["controllers"];
  
        controllers_button_incrementer = controllers["button_incrementer"];
        controllers_button_incrementer_controller_name = strdup(controllers_button_incrementer["controller_name"]);
        controllers_button_incrementer_controller_goal = controllers_button_incrementer["controller_goal"];
        controllers_button_incrementer_number = controllers_button_incrementer["number"]; // 1
  
        incrementer.set_button(button1);
        incrementer.set_name(controllers_button_incrementer_controller_name);
        incrementer.set_goal(controllers_button_incrementer_controller_goal);
  
        controllers_button_toggler = controllers["button_toggler"];
        controllers_button_toggler_controller_name = strdup(controllers_button_toggler["controller_name"]);
        controllers_button_toggler_controller_goal = controllers_button_toggler["controller_goal"]; // -1
        controllers_button_toggler_number = controllers_button_incrementer["number"];               // 1
  
        toggler.set_button(button2);
        toggler.set_name(controllers_button_toggler_controller_name);
        toggler.set_goal(controllers_button_toggler_controller_goal);
        toggler.set_num_states(2);
  
        controllers_button_led = controllers["button_led"];
        controllers_button_led_controller_name = strdup(controllers_button_led["controller_name"]);
        controllers_button_led_controller_goal = controllers_button_led["controller_goal"]; // -1
        controllers_button_led_number = controllers_button_incrementer["number"];           // 1
  
        button_led.set_button(button3);
        button_led.set_name(controllers_button_led_controller_name);
        button_led.set_goal(controllers_button_led_controller_goal);
        button_led.set_color_channels(PWM_CHANNEL_R, PWM_CHANNEL_G, PWM_CHANNEL_B);
  
        // TODO - Implement microphone and shaker
        //      JsonObject controllers_microphone = controllers["microphone"];
        //      char* controllers_microphone_controller_name = strdup(controllers_microphone["controller_name"]);
        //      int controllers_microphone_controller_goal = controllers_microphone["controller_goal"]; // -1
        //      int controllers_microphone_number = controllers_button_incrementer["number"]; // 1
        //
        //      JsonObject controllers_shaker = controllers["shaker"];
        //      char* controllers_shaker_controller_name = strdup(controllers_shaker["controller_name"]); // "Scadoodle"
        //      int controllers_shaker_controller_goal = controllers_shaker["controller_goal"]; // -1
        //      int controllers_microphone_number = controllers_button_incrementer["number"]; // 1
        //      display
        tft.setCursor(0, 0, 1);
        tft.fillScreen(TFT_BLACK);
        tft.println(task);
        tft.println("---------------------");
        tft.println(incrementer.get_name());
  
        if (incrementer.is_complete())
        {
          tft.println("Success");
          prev_incrementer = true;
        }
        else
        {
          tft.println(incrementer.get_state());
        }
  
        tft.println("---------------------");
  
        tft.println(toggler.get_name());
        if (toggler.is_complete())
        {
          tft.println("Success");
          prev_toggler = true;
        }
        else
        {
          tft.println(toggler.get_state());
        }
  
        tft.println("---------------------");
  
        tft.println(button_led.get_name());
        if (button_led.is_complete())
        {
          tft.println("Success");
          prev_led = true;
        }
        else
        {
          tft.println(button_led.get_state());
        }
      }
    }
  }
}

void loop()
{
  game_status = false;
  if (game_over) return;
  //   game loop
  //  checks if round is still going and updates controllers if new round
  if (millis() - primary_timer >= LOOP_INTERVAL)
  {
    primary_timer = millis();
    char *json_response = get_new_round(game_id, player_name, round_num, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT);

    //  parse JSON here
    StaticJsonDocument<2000> doc;

    DeserializationError error = deserializeJson(doc, json_response);

    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    // TODO - for now we are only doing 1 round so no reinitialization of the controllers and tasks etc
    game_status = doc["status"];  // true
    if (game_status) {//if it's true, we got some new controls, and we have to update, else not
      const char* task = doc["task"];           // "Increment the scadoodle to 10"
      task_display = (char*)task;
      round_num = doc["round_num"]; // round number
      JsonObject controllers = doc["controllers"];
  
      JsonObject controllers_button_incrementer = controllers["button_incrementer"];
      controllers_button_incrementer_controller_name = strdup(controllers_button_incrementer["controller_name"]);
      controllers_button_incrementer_controller_goal = controllers_button_incrementer["controller_goal"];
      controllers_button_incrementer_number = controllers_button_incrementer["number"]; // 1
  
      JsonObject controllers_button_toggler = controllers["button_toggler"];
      controllers_button_toggler_controller_name = strdup(controllers_button_toggler["controller_name"]);
      controllers_button_toggler_controller_goal = controllers_button_toggler["controller_goal"]; // -1
      controllers_button_toggler_number = controllers_button_incrementer["number"];               // 1
  
      JsonObject controllers_button_led = controllers["button_led"];
      controllers_button_led_controller_name = strdup(controllers_button_led["controller_name"]);
      controllers_button_led_controller_goal = controllers_button_led["controller_goal"]; // -1
      controllers_button_led_number = controllers_button_incrementer["number"];           //
    }
  }

  //  if the round is running then update the controllers (check if buttons pressed, etc)
  if (!game_status)
  {
    if (controllers_button_incrementer_number != -1)
    {
      incrementer_updated = incrementer.update();
    }
    if (controllers_button_toggler_number != -1)
    {
      button_toggler_updated = toggler.update();
    }
    if (controllers_button_led_number != -1)
    {
      button_led_updated = button_led.update();
    }

    // if a controller was updated, then show them to client - this will be updated to incorporate UI next week
    if ((incrementer.is_complete() && incrementer.is_updated()) || (toggler.is_complete() && toggler.is_updated()) || (button_led.is_complete() && button_led.is_updated()) || (incrementer_updated || button_toggler_updated || button_led_updated))
    {
      Serial.println("SOMETHING WAS UPDATED!");
      tft.setCursor(0, 0, 1);
      tft.fillScreen(TFT_BLACK);
      tft.println(task_display);
      tft.println("---------------------");
      tft.println(incrementer.get_name());

      if (incrementer.is_complete())
      {
        tft.println("Success");
        prev_incrementer = true;
      }
      else
      {
        tft.println(incrementer.get_state());
      }

      tft.println("---------------------");

      tft.println(toggler.get_name());
      if (toggler.is_complete())
      {
        tft.println("Success");
        prev_toggler = true;
      }
      else
      {
        tft.println(toggler.get_state());
      }

      tft.println("---------------------");

      tft.println(button_led.get_name());
      if (button_led.is_complete())
      {
        tft.println("Success");
        prev_led = true;
      }
      else
      {
        tft.println(button_led.get_state());
      }
    }
    if (incrementer.is_complete()) {
      Serial.print("INCREMENTER IS COMPLETE BRUHHHHH");
    }

    //  after updating the client view, post any successes to the backend
    if (incrementer.is_complete() || toggler.is_complete() || button_led.is_complete())
    {
      post_completed_task(game_id, player_name, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT);
    }
  }
  //  Since we are only doing one round at the moment, if the game_status is true(new round info sent) then the game ended! :)
  else
  {
    tft.println("Game has ended! Nice job!");
    game_over = true;
  }
}

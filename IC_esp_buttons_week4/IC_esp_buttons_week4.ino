#include <WiFi.h> //Connect to WiFi Network
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <mpu6050_esp32.h>
#include <math.h>
#include <string.h>
#include "shake.h"
#include "microphone.h"
#include "button_incrementer.h"
#include "button_toggler.h"
#include "button_led.h"
#include "button_class.h"

const char *CA_CERT =
  "-----BEGIN CERTIFICATE-----\n"
  "MIIDujCCAqKgAwIBAgILBAAAAAABD4Ym5g0wDQYJKoZIhvcNAQEFBQAwTDEgMB4G\n"
  "A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNp\n"
  "Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDYxMjE1MDgwMDAwWhcNMjExMjE1\n"
  "MDgwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEG\n"
  "A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n"
  "hvcNAQEBBQADggEPADCCAQoCggEBAKbPJA6+Lm8omUVCxKs+IVSbC9N/hHD6ErPL\n"
  "v4dfxn+G07IwXNb9rfF73OX4YJYJkhD10FPe+3t+c4isUoh7SqbKSaZeqKeMWhG8\n"
  "eoLrvozps6yWJQeXSpkqBy+0Hne/ig+1AnwblrjFuTosvNYSuetZfeLQBoZfXklq\n"

  "C9XZzPnqJworc5HGnRusyMvo4KD0L5CLTfuwNhv2GXqF4G3yYROIXJ/gkwpRl4pa\n"
  "zq+r1feqCapgvdzZX99yqWATXgAByUr6P6TqBwMhAo6CygPCm48CAwEAAaOBnDCB\n"
  "mTAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUm+IH\n"
  "V2ccHsBqBt5ZtJot39wZhi4wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDovL2NybC5n\n"
  "bG9iYWxzaWduLm5ldC9yb290LXIyLmNybDAfBgNVHSMEGDAWgBSb4gdXZxwewGoG\n"
  "3lm0mi3f3BmGLjANBgkqhkiG9w0BAQUFAAOCAQEAmYFThxxol4aR7OBKuEQLq4Gs\n"
  "J0/WwbgcQ3izDJr86iw8bmEbTUsp9Z8FHSbBuOmDAGJFtqkIk7mpM0sYmsL4h4hO\n"
  "291xNBrBVNpGP+DTKqttVCL1OmLNIG+6KYnX3ZHu01yiPqFbQfXf5WRDLenVOavS\n"
  "ot+3i9DAgBkcRcAtjOj4LaR0VknFBbVPFd5uRHg5h6h+u/N5GJG79G+dwfCMNYxd\n"
  "AfvDbbnvRG15RjF+Cv6pgsH/76tuIMRQyV+dTZsXjAzlAcmgQWpzU/qlULRuJQ/7\n"
  "TBj0/VLZjmmx6BEP3ojY+x1J96relc8geMJgEtslQIxq/H5COEBkEveegeGTLg==\n"
  "-----END CERTIFICATE-----\n";

// ESP constants
TFT_eSPI tft = TFT_eSPI();
const int SCREEN_HEIGHT = 160;
const int SCREEN_WIDTH = 128;
const int PIN1 = 5;  //incrementer
const int PIN2 = 12; //toggler
const int PIN3 = 13; //button-led
const int PIN4 = 0;

// LED constants
const int R_PIN = 32;
const int G_PIN = 33;
const int B_PIN = 27;
const uint32_t PWM_CHANNEL_R = 0; //hardware pwm channel
const uint32_t PWM_CHANNEL_G = 1; //hardware pwm channel
const uint32_t PWM_CHANNEL_B = 2; //hardware pwm channel

// const int LOOP_PERIOD = 40;

// Network constants
char network[] = "walhallaP";
char password[] = "modernvase639";

const int RESPONSE_TIMEOUT = 6000;     //ms to wait for response from host
const uint16_t OUT_BUFFER_SIZE = 1500; //size of buffer to hold HTTP response
char request[OUT_BUFFER_SIZE];
char old_response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response[OUT_BUFFER_SIZE];     //char array buffer to hold HTTP request

//microphone constants
const int DELAY = 1000;
const int SAMPLE_FREQ = 8000;                                            // Hz, telephone sample rate
const int SAMPLE_DURATION = 3;                                           // duration of fixed sampling (seconds)
const int NUM_SAMPLES = SAMPLE_FREQ * SAMPLE_DURATION;                   // number of of samples
const int ENC_LEN = (NUM_SAMPLES + 2 - ((NUM_SAMPLES + 2) % 3)) / 3 * 4; // Encoded length of clip

//Prefix to POST request:
const char PREFIX[] = "{\"config\":{\"encoding\":\"MULAW\",\"sampleRateHertz\":8000,\"languageCode\": \"en-US\", \"speechContexts\":{\"phrases\":[\"red\", \"green\", \"blue\", \"square\", \"circle\", \"password\"]}}, \"audio\": {\"content\":\"";
const char SUFFIX[] = "\"}}";                                     //suffix to POST request
const int AUDIO_IN = A0;                                          //pin where microphone is connected
const char API_KEY[] = "AIzaSyCwyynsePu7xijUYTOgR7NdVqxH2FAG9DQ"; //don't change this
char transcript[100];

uint8_t button_state;       //used for containing button state and detecting edges
int old_button_state;       //used for detecting button edges
uint32_t time_since_sample; // used for microsecond timing

char speech_data[ENC_LEN + 200] = {0};    //global used for collecting speech data
const char *SERVER = "speech.google.com"; // Server URL
uint8_t old_val;
uint32_t timer;
uint32_t secondary_timer;

// other
uint32_t primary_timer;
uint32_t LOOP_INTERVAL = 5000;
bool prev_incrementer;
bool prev_toggler;
bool prev_led;

bool initialize;

MPU6050 imu;
Button button1(PIN1); //button object!
Button button2(PIN2); //button object!
Button button3(PIN3); //button object!
Button button4(PIN4); //button object!

// Each player fills this in before run (in future will make user input)
char *game_id = "6867";
char *player_name = "diego";
int round_num = 0;
//char* json_response;

WiFiClientSecure client;
Microphone microphone = Microphone(tft);
Button_Incrementer incrementer = Button_Incrementer(tft);
Button_Toggler toggler = Button_Toggler(tft);
Button_LED button_led = Button_LED(tft);
Shake shaker = Shake(tft);

bool new_round = false;
char *task_display;
char *controllers_button_incrementer_controller_name;
int controllers_button_incrementer_controller_goal;
int controllers_button_incrementer_number;
char *controllers_button_toggler_controller_name;
int controllers_button_toggler_controller_goal;
int controllers_button_toggler_number;
char *controllers_button_led_controller_name;
int controllers_button_led_controller_goal;
int controllers_button_led_number;

char *controllers_microphone_controller_name;
int controllers_microphone_controller_goal;
int controllers_microphone_number;
char *controllers_shaker_controller_name;
int controllers_shaker_controller_goal;
int controllers_shaker_number;
bool sent_success = false;
bool new_round_req_loop = false;
bool leave_check_start = false;

char *task;
char *end_text;

bool incrementer_updated = false;
bool button_toggler_updated = false;
bool button_led_updated = false;
bool microphone_updated = false;
bool shaker_updated = false;
bool round_started = false;
bool game_over = false;

char *check_start(char *game_id, char *player_name, char *request_buffer, char *response, uint16_t response_size, uint16_t response_timeout);

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

  pinMode(PIN1, INPUT_PULLUP);
  pinMode(PIN2, INPUT_PULLUP);
  pinMode(PIN3, INPUT_PULLUP);
  pinMode(PIN4, INPUT_PULLUP);

  primary_timer = millis();
  secondary_timer = millis();
  initialize = false;

  client.setCACert(CA_CERT);
  old_val = digitalRead(PIN4);
  button_state = 1;

  if (imu.setupIMU(1))
  {
    Serial.println("IMU Connected!");
  }
  else
  {
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); //set color of font to green foreground, black background
  tft.setCursor(0, 0, 1);
}

void loop()
{
  //end loop
  if (game_over)
  {
    tft.fillScreen(TFT_BLACK);

    tft.println(end_text); // gets score
    tft.println("Press a button to join a new game");
    //      blocking
    while (!(button1.update() || button2.update() || button3.update() || button4.update()))
    {
    }
    initialize = false;
    sent_success = false;
    game_over = false;
    new_round_req_loop = false;
    return;
  };

  //check start loop ... blocking
  if (!initialize)
  {

    // TEMPORARY: clear the db before initializing this player
    // get_clear(request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT);

    GameIDInput gameID;

    while (gameID.getState() != 2) {
        gameID.update(button1, etc); 
    }
    
//    tft.fillScreen(TFT_BLACK);
//    tft.println("Press any button to indicate that you're ready!");
//    Serial.println("Waiting for button to be pressed");
//    // blocking waiting room
//    
//    while (!(button1.update() || button2.update() || button3.update() || button4.update()))
//    {
//    }

    // post_ready_to_play(game_id, player_name, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT);
    // indicate ready to play

    // removed first request from here
    while (!leave_check_start)
    {

      if (millis() - primary_timer >= LOOP_INTERVAL)
      {
        tft.setCursor(0, 0, 1);
        tft.fillScreen(TFT_BLACK);
        tft.println("Waiting for other players");
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

        leave_check_start = (strcmp(doc["status"], "update") == 0); // true means that the round has just loaded
        if (leave_check_start)
        {
          sent_success = false;
          // display setup
          tft.fillScreen(TFT_BLACK);
          tft.drawLine(64, 10, 64, 160, TFT_WHITE);
          tft.drawLine(0, 10, 128, 10, TFT_WHITE);
          tft.drawLine(0, 80, 128, 80, TFT_WHITE);

          task = strdup(doc["text"]); // "Increment the scadoodle to 10"
          Serial.println(task);
          JsonObject controllers = doc["controllers"];
          round_num = doc["round_num"].as<int>();

          JsonObject controllers_button_incrementer = controllers["button-increment"].as<JsonObject>();
          controllers_button_incrementer_controller_name = (char *)controllers_button_incrementer["controller_name"].as<const char *>();
          controllers_button_incrementer_controller_goal = controllers_button_incrementer["controller_goal"];
          controllers_button_incrementer_number = controllers_button_incrementer["number"]; // 1
          incrementer.set_button(button1);
          incrementer.set_name(controllers_button_incrementer_controller_name);
          incrementer.set_goal(controllers_button_incrementer_controller_goal);
          Serial.println(controllers_button_incrementer_controller_goal);
          incrementer.set_quadrant(controllers_button_incrementer_number);

          JsonObject controllers_button_toggler = controllers["button-toggle"].as<JsonObject>();
          controllers_button_toggler_controller_name = (char *)controllers_button_toggler["controller_name"].as<const char *>();
          controllers_button_toggler_controller_goal = controllers_button_toggler["controller_goal"]; // -1
          controllers_button_toggler_number = controllers_button_toggler["number"];                   // 1

          toggler.set_button(button2);
          toggler.set_name(controllers_button_toggler_controller_name);
          toggler.set_goal(controllers_button_toggler_controller_goal);
          Serial.println(controllers_button_toggler_controller_goal);
          toggler.set_num_states(2);
          toggler.set_quadrant(controllers_button_toggler_number);

          JsonObject controllers_button_led = controllers["button-LED-toggle"].as<JsonObject>();
          controllers_button_led_controller_name = (char *)controllers_button_led["controller_name"].as<const char *>();
          controllers_button_led_controller_goal = controllers_button_led["controller_goal"]; // -1
          controllers_button_led_number = controllers_button_led["number"];                   // 1

          button_led.set_button(button3);
          button_led.set_name(controllers_button_led_controller_name);
          button_led.set_goal(controllers_button_led_controller_goal);
          Serial.println(controllers_button_led_controller_goal);
          button_led.set_color_channels(PWM_CHANNEL_R, PWM_CHANNEL_G, PWM_CHANNEL_B);
          button_led.set_quadrant(controllers_button_led_number);

          JsonObject controllers_microphone = controllers["microphone-password"].as<JsonObject>();
          controllers_microphone_controller_name = (char *)controllers_microphone["controller_name"].as<const char *>();
          controllers_microphone_controller_goal = controllers_microphone["controller_goal"]; // -1
          controllers_microphone_number = controllers_microphone["number"];                   // 1

          microphone.set_name(controllers_microphone_controller_name);
          microphone.set_goal(controllers_microphone_controller_goal);
          Serial.println(controllers_microphone_controller_goal);
          microphone.set_quadrant(controllers_microphone_number);

          JsonObject controllers_shaker = controllers["device-shake"].as<JsonObject>();
          controllers_shaker_controller_name = (char *)controllers_shaker["controller_name"].as<const char *>(); // "Scadoodle"
          controllers_shaker_controller_goal = controllers_shaker["controller_goal"];                            // -1
          controllers_shaker_number = controllers_shaker["number"];                                              // 1

          shaker.set_name(controllers_shaker_controller_name);
          shaker.set_goal(controllers_shaker_controller_goal);
          Serial.println(controllers_shaker_controller_goal);
          shaker.set_quadrant(controllers_shaker_number);

          Serial.println("done making");
        }

        bool msg = (strcmp(doc["status"], "static") == 0 || strcmp(doc["status"], "error") == 0);
        if (msg)
        {
          Serial.print(F("Message from server: "));
          serializeJson(doc["text"], Serial);
          Serial.println();
        }

        doc.clear();
        Serial.println("cleared doc");
      }
    }

    tft.setCursor(0, 0, 1);
    tft.println(task);
    shaker.draw(false);
    Serial.println("shaker drawn");
    microphone.draw(false);
    Serial.println("mic drawn");
    toggler.draw(false);
    Serial.println("toggler drawn");
    button_led.draw(false);
    Serial.println("buttonLED drawn");
    incrementer.draw(false);
    Serial.println("increm drawn");

    initialize = true;
    Serial.println("initialized");
  }

  //   game loop
  //  if we previously logged a sent task, now we wait until we get an update response
  if (new_round_req_loop)
  {
    if (millis() - secondary_timer >= LOOP_INTERVAL)
    {
      char *json_response = get_new_round(game_id, player_name, round_num, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT);
      Serial.println("diego is lame");
      Serial.println(json_response);
      Serial.println("diego is lame");
      secondary_timer = millis();

      //  parse JSON here
      StaticJsonDocument<2000> doc;

      DeserializationError error = deserializeJson(doc, json_response);

      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      new_round = (strcmp(doc["status"], "update") == 0);
      if (new_round)
      { //if it's true, we got some new controls, and we have to update, else not
        task = strdup(doc["text"]); // "Increment the scadoodle to 10"
        JsonObject controllers = doc["controllers"];
        round_num = doc["round_num"].as<int>();

        JsonObject controllers_button_incrementer = controllers["button-increment"].as<JsonObject>();
        controllers_button_incrementer_controller_name = (char *)controllers_button_incrementer["controller_name"].as<const char *>();
        controllers_button_incrementer_controller_goal = controllers_button_incrementer["controller_goal"];
        controllers_button_incrementer_number = controllers_button_incrementer["number"]; // 1
        incrementer.set_button(button1);
        incrementer.set_name(controllers_button_incrementer_controller_name);
        incrementer.set_goal(controllers_button_incrementer_controller_goal);
        incrementer.set_quadrant(controllers_button_incrementer_number);

        JsonObject controllers_button_toggler = controllers["button-toggle"].as<JsonObject>();
        controllers_button_toggler_controller_name = (char *)controllers_button_toggler["controller_name"].as<const char *>();
        controllers_button_toggler_controller_goal = controllers_button_toggler["controller_goal"]; // -1
        controllers_button_toggler_number = controllers_button_toggler["number"];                   // 1

        toggler.set_button(button2);
        toggler.set_name(controllers_button_toggler_controller_name);
        toggler.set_goal(controllers_button_toggler_controller_goal);
        toggler.set_num_states(2);
        toggler.set_quadrant(controllers_button_toggler_number);

        JsonObject controllers_button_led = controllers["button-LED-toggle"].as<JsonObject>();
        controllers_button_led_controller_name = (char *)controllers_button_led["controller_name"].as<const char *>();
        controllers_button_led_controller_goal = controllers_button_led["controller_goal"]; // -1
        controllers_button_led_number = controllers_button_led["number"];                   // 1

        button_led.set_button(button3);
        button_led.set_name(controllers_button_led_controller_name);
        button_led.set_goal(controllers_button_led_controller_goal);
        button_led.set_color_channels(PWM_CHANNEL_R, PWM_CHANNEL_G, PWM_CHANNEL_B);
        button_led.set_quadrant(controllers_button_led_number);

        JsonObject controllers_microphone = controllers["microphone-password"].as<JsonObject>();
        controllers_microphone_controller_name = (char *)controllers_microphone["controller_name"].as<const char *>();
        controllers_microphone_controller_goal = controllers_microphone["controller_goal"]; // -1
        controllers_microphone_number = controllers_microphone["number"];                   // 1

        microphone.set_name(controllers_microphone_controller_name);
        microphone.set_goal(controllers_microphone_controller_goal);
        microphone.set_quadrant(controllers_microphone_number);

        JsonObject controllers_shaker = controllers["device-shake"].as<JsonObject>();
        controllers_shaker_controller_name = (char *)controllers_shaker["controller_name"].as<const char *>(); // "Scadoodle"
        controllers_shaker_controller_goal = controllers_shaker["controller_goal"];                            // -1
        controllers_shaker_number = controllers_shaker["number"];                                              // 1

        shaker.set_name(controllers_shaker_controller_name);
        shaker.set_goal(controllers_shaker_controller_goal);
        shaker.set_quadrant(controllers_shaker_number);

        tft.fillScreen(TFT_BLACK);
        tft.drawLine(64, 10, 64, 160, TFT_WHITE);
        tft.drawLine(0, 10, 128, 10, TFT_WHITE);
        tft.drawLine(0, 80, 128, 80, TFT_WHITE);

        tft.setCursor(0, 0, 1);
        tft.println(task);
        shaker.draw(false);
        Serial.println("shaker drawn");
        microphone.draw(false);
        Serial.println("mic drawn");
        toggler.draw(false);
        Serial.println("toggler drawn");
        button_led.draw(false);
        Serial.println("buttonLED drawn");
        incrementer.draw(false);
        Serial.println("increm drawn");

        sent_success = false; //only want to do this when task is completed
      }

      new_round_req_loop = (strcmp(doc["status"], "static") == 0 || strcmp(doc["status"], "error") == 0);
      if (new_round_req_loop)
      {

        Serial.print("Message from server: ");
        serializeJson(doc["text"], Serial);
        Serial.println();
      }

      game_over = (strcmp(doc["status"], "over") == 0);
      if (game_over)
      {
        const char *end_text_temp = doc["text"];
        end_text = (char *)end_text_temp;
        return;
      }
      //only one of new_round, new_round_req_loop, and game_over can be true at a time
      doc.clear();
    }
  }

  //  wait for task completion loop
  if (!new_round_req_loop)
  {
    if (millis() - secondary_timer >= LOOP_INTERVAL)
    {
      char *json_response = get_new_round(game_id, player_name, round_num, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT);
      //  parse JSON here
      secondary_timer = millis();
      StaticJsonDocument<2000> doc;
      DeserializationError error = deserializeJson(doc, json_response);

      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      game_over = (strcmp(doc["status"], "over") == 0);
      if (game_over)
      {
        const char *end_text_temp = doc["text"];
        end_text = (char *)end_text_temp;
        return;
      }
      //only one of new_round, new_round_req_loop, and game_over can be true at a time
      doc.clear();
    }

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
    if (controllers_microphone_number != -1)
    {
      button_state = digitalRead(PIN4);
      if (button_state == 0)
      {
        Serial.println("listening...");
        record_audio();
        Serial.println("sending...");
        Serial.print("\nStarting connection to server...");
        delay(6000);
        microphone.draw(microphone.is_complete("password"));
        bool conn = false;
        for (int i = 0; i < 10; i++)
        {
          int val = (int)client.connect(SERVER, 443);
          Serial.print(i);
          Serial.print(": ");
          Serial.println(val);
          if (val != 0)
          {
            conn = true;
            break;
          }
          Serial.print(".");
          delay(300);
        }
        if (!conn)
        {
          Serial.println("Connection failed!");
          return;
        }
        else
        {
          Serial.println("Connected to server!");
          Serial.println(client.connected());
          int len = strlen(speech_data);
          // Make a HTTP request:
          client.print("POST /v1/speech:recognize?key=");
          client.print(API_KEY);
          client.print(" HTTP/1.1\r\n");
          client.print("Host: speech.googleapis.com\r\n");
          client.print("Content-Type: application/json\r\n");
          client.print("cache-control: no-cache\r\n");
          client.print("Content-Length: ");
          client.print(len);
          client.print("\r\n\r\n");
          int ind = 0;
          int jump_size = 1000;
          char temp_holder[jump_size + 10] = {0};
          Serial.println("sending data");
          while (ind < len)
          {
            delay(80); //experiment with this number!
            //if (ind + jump_size < len) client.print(speech_data.substring(ind, ind + jump_size));
            strncat(temp_holder, speech_data + ind, jump_size);
            client.print(temp_holder);
            ind += jump_size;
            memset(temp_holder, 0, sizeof(temp_holder));
          }
          client.print("\r\n");
          //Serial.print("\r\n\r\n");
          Serial.println("Through send...");
          unsigned long count = millis();
          while (client.connected())
          {
            Serial.println("IN!");
            String line = client.readStringUntil('\n');
            Serial.print(line);
            if (line == "\r")
            { //got header of response
              Serial.println("headers received");
              break;
            }
            if (millis() - count > RESPONSE_TIMEOUT)
              break;
          }
          Serial.println("");
          Serial.println("Response...");
          count = millis();
          while (!client.available())
          {
            delay(100);
            Serial.print(".");
            if (millis() - count > RESPONSE_TIMEOUT)
              break;
          }
          Serial.println();
          Serial.println("-----------");
          memset(response, 0, sizeof(response));
          while (client.available())
          {
            char_append(response, client.read(), OUT_BUFFER_SIZE);
          }
          Serial.println(response);
          char *trans_id = strstr(response, "transcript");
          if (trans_id != NULL)
          {
            char *foll_coll = strstr(trans_id, ":");
            char *starto = foll_coll + 2;          //starting index
            char *endo = strstr(starto + 1, "\""); //ending index
            int transcript_len = endo - starto + 1;
            transcript[0] = 0;
            strncat(transcript, starto, transcript_len);
            Serial.println(transcript);
            microphone.draw(microphone.is_complete(transcript));
          }
          Serial.println("-----------");
          client.stop();
          Serial.println("done");
        }
        old_button_state = button_state;
      }
    }
    if (controllers_shaker_number != -1)
    {
      // shaker:
      imu.readAccelData(imu.accelCount);
      float x, y, z;
      x = imu.accelCount[0] * imu.aRes;
      y = imu.accelCount[1] * imu.aRes;
      z = imu.accelCount[2] * imu.aRes;
      float acc_mag = sqrt(x * x + y * y + z * z);
      shaker_updated = shaker.update(acc_mag);
    }

    // if a controller was updated, then show them to client - this will be updated to incorporate UI next week
    if ((incrementer.is_complete() || incrementer_updated) && !sent_success)
    {
      Serial.println("Draw incrementer");
      incrementer.draw(incrementer.is_complete());
    }
    if ((toggler.is_complete() || button_toggler_updated) && !sent_success)
    {
      Serial.println("Draw toggler");
      toggler.draw(toggler.is_complete());
    }
    if ((button_led.is_complete() || button_led_updated) && !sent_success)
    {
      Serial.println("Draw led");

      button_led.draw(button_led.is_complete());
    }
    if ((microphone.is_complete(transcript) || microphone_updated) && !sent_success)
    {
      microphone.draw(microphone.is_complete(transcript));
    }
    if ((shaker.is_complete() || shaker_updated) && !sent_success)
    {
      shaker.draw(shaker.is_complete());
    }

    //  after updating the client view, post any successes to the backend
    if (incrementer.is_complete() || toggler.is_complete() || button_led.is_complete() || microphone.is_complete(transcript) || shaker.is_complete() && !sent_success)
    {
      post_completed_task(game_id, player_name, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT);
      sent_success = true;
      new_round_req_loop = true;
    }
  }
}

// --------------------------------- SUPPORT FUNCTIONS -----------------------------------------

//function used to record audio at sample rate for a fixed nmber of samples
void record_audio()
{
  int sample_num = 0;                 // counter for samples
  int enc_index = strlen(PREFIX) - 1; // index counter for encoded samples
  float time_between_samples = 1000000 / SAMPLE_FREQ;
  int value = 0;
  char raw_samples[3]; // 8-bit raw sample data array
  memset(speech_data, 0, sizeof(speech_data));
  sprintf(speech_data, "%s", PREFIX);
  char holder[5] = {0};
  Serial.println("starting");
  uint32_t text_index = enc_index;
  uint32_t start = millis();
  time_since_sample = micros();
  while (sample_num < NUM_SAMPLES && millis() - start < 5000)
  { //read in NUM_SAMPLES worth of audio data
    value = analogRead(AUDIO_IN);                             //make measurement
    raw_samples[sample_num % 3] = mulaw_encode(value - 1551); //remove 1.25V offset (from 12 bit reading)
    sample_num++;
    if (sample_num % 3 == 0)
    {
      base64_encode(holder, raw_samples, 3);
      strncat(speech_data + text_index, holder, 4);
      text_index += 4;
    }
    // wait till next time to read
    while (micros() - time_since_sample <= time_between_samples)
      ; //wait...
    time_since_sample = micros();
  }
  Serial.println(millis() - start);
  sprintf(speech_data + strlen(speech_data), "%s", SUFFIX);
  Serial.println("out");
}

int8_t mulaw_encode(int16_t sample)
{
  const uint16_t MULAW_MAX = 0x1FFF;
  const uint16_t MULAW_BIAS = 33;
  uint16_t mask = 0x1000;
  uint8_t sign = 0;
  uint8_t position = 12;
  uint8_t lsb = 0;
  if (sample < 0)
  {
    sample = -sample;
    sign = 0x80;
  }
  sample += MULAW_BIAS;
  if (sample > MULAW_MAX)
  {
    sample = MULAW_MAX;
  }
  for (; ((sample & mask) != mask && position >= 5); mask >>= 1, position--)
    ;
  lsb = (sample >> (position - 4)) & 0x0f;
  return (~(sign | ((position - 5) << 4) | lsb));
}

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

// void post_ready_to_play(char *game_id, char *player_name, char *request_buffer, char *response, uint16_t response_size, uint16_t response_timeout)
// {
//   char body[100];                                                                            //for body
//   sprintf(body, "game_id=%s&user_id=%s", game_id, player_name);                              //generate body
//   int body_len = strlen(body);                                                               //calculate body length (for header reporting)
//   sprintf(request_buffer, "POST https://shipgroups.herokuapp.com/user_ready/ HTTP/1.1\r\n"); // TODO!!!!!
//   strcat(request_buffer, "Host: shipgroups.herokuapp.com\r\n");
//   strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
//   sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
//   strcat(request_buffer, "\r\n");                                                       //new line from header to body
//   strcat(request_buffer, body);                                                         //body
//   strcat(request_buffer, "\r\n");                                                       //new line
//   Serial.println(request_buffer);
//   do_http_request("shipgroups.herokuapp.com", request_buffer, response, response_size, response_timeout, true);
//   Serial.println(response); //viewable in Serial Terminal
// }

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
  char body[100];                                                                               //for body
  sprintf(body, "game_id=%s&user_id=%s", game_id, player_name);                                 //generate body
  int body_len = strlen(body);                                                                  //calculate body length (for header reporting)
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

const char PROGMEM b64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                    "abcdefghijklmnopqrstuvwxyz"
                                    "0123456789+/";

inline void a3_to_a4(unsigned char *a4, unsigned char *a3);
inline void a4_to_a3(unsigned char *a3, unsigned char *a4);
inline unsigned char b64_lookup(char c);

int base64_encode(char *output, char *input, int inputLen)
{
  int i = 0, j = 0;
  int encLen = 0;
  unsigned char a3[3];
  unsigned char a4[4];

  while (inputLen--)
  {
    a3[i++] = *(input++);
    if (i == 3)
    {
      a3_to_a4(a4, a3);

      for (i = 0; i < 4; i++)
      {
        output[encLen++] = pgm_read_byte(&b64_alphabet[a4[i]]);
      }

      i = 0;
    }
  }

  if (i)
  {
    for (j = i; j < 3; j++)
    {
      a3[j] = '\0';
    }

    a3_to_a4(a4, a3);

    for (j = 0; j < i + 1; j++)
    {
      output[encLen++] = pgm_read_byte(&b64_alphabet[a4[j]]);
    }

    while ((i++ < 3))
    {
      output[encLen++] = '=';
    }
  }
  //  output[encLen] = '\0';
  return encLen;
}

int base64_decode(char *output, char *input, int inputLen)
{
  int i = 0, j = 0;
  int decLen = 0;
  unsigned char a3[3];
  unsigned char a4[4];

  while (inputLen--)
  {
    if (*input == '=')
    {
      break;
    }

    a4[i++] = *(input++);
    if (i == 4)
    {
      for (i = 0; i < 4; i++)
      {
        a4[i] = b64_lookup(a4[i]);
      }

      a4_to_a3(a3, a4);

      for (i = 0; i < 3; i++)
      {
        output[decLen++] = a3[i];
      }
      i = 0;
    }
  }

  if (i)
  {
    for (j = i; j < 4; j++)
    {
      a4[j] = '\0';
    }

    for (j = 0; j < 4; j++)
    {
      a4[j] = b64_lookup(a4[j]);
    }

    a4_to_a3(a3, a4);

    for (j = 0; j < i - 1; j++)
    {
      output[decLen++] = a3[j];
    }
  }
  output[decLen] = '\0';
  return decLen;
}

int base64_enc_len(int plainLen)
{
  int n = plainLen;
  return (n + 2 - ((n + 2) % 3)) / 3 * 4;
}

int base64_dec_len(char *input, int inputLen)
{
  int i = 0;
  int numEq = 0;
  for (i = inputLen - 1; input[i] == '='; i--)
  {
    numEq++;
  }

  return ((6 * inputLen) / 8) - numEq;
}

inline void a3_to_a4(unsigned char *a4, unsigned char *a3)
{
  a4[0] = (a3[0] & 0xfc) >> 2;
  a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
  a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
  a4[3] = (a3[2] & 0x3f);
}

inline void a4_to_a3(unsigned char *a3, unsigned char *a4)
{
  a3[0] = (a4[0] << 2) + ((a4[1] & 0x30) >> 4);
  a3[1] = ((a4[1] & 0xf) << 4) + ((a4[2] & 0x3c) >> 2);
  a3[2] = ((a4[2] & 0x3) << 6) + a4[3];
}

inline unsigned char b64_lookup(char c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return c - 71;
  if (c >= '0' && c <= '9')
    return c + 4;
  if (c == '+')
    return 62;
  if (c == '/')
    return 63;
  return -1;
}

class GameIDInput {
    char alphabet[50] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char msg[400] = {0}; //contains previous query response
    char query_string[50] = {0};
    int char_index;
    int state;
    uint32_t scrolling_timer;
    const int scrolling_threshold = 150;
    const float angle_threshold = 0.3;
    uint32_t timer;
    float prev_stonk_val;
  public:
    GameIDInput() {
      state = 0;
      memset(msg, 0, sizeof(msg));
      strcat(msg, "Long Press to Start Entering Game ID");
      char_index = 0;
      scrolling_timer = millis();
    }
    void update(float angle, int button, char* output) {
      char string[2];
      if (state == 0) {
        memset(output, 0, sizeof(output));
        strcat(output, msg);
        if (button == 2) {
          state = 1;
          char_index = 0;
          memset(query_string, 0, sizeof(query_string));
          strcat(query_string, "");
          scrolling_timer = millis();
        }
      } else if (state == 1) {
        if (button == 1) {
          string[0] = alphabet[char_index];
          string[1] = '\0';
          strcat(query_string, string);
          memset(output, 0, sizeof(output));
          strcat(output, query_string);
          char_index = 0;
        } else if (button == 2) {
          state = 2;
          char_index = 0;
          memset(output, 0, sizeof(output));
          strcat(output, "");
        } else if (millis() - scrolling_timer >= scrolling_threshold) {
          if (angle >= angle_threshold) {
            scrolling_timer = millis();
            char_index += 1;
            while (char_index < 0) {
              char_index += strlen(alphabet);
            }
            char_index %= strlen(alphabet);
          } else if (angle <= -angle_threshold) {
            scrolling_timer = millis();
            char_index -= 1;
            while (char_index < 0) {
              char_index += strlen(alphabet);
            }
            char_index %= strlen(alphabet);
          }


          memset(output, 0, sizeof(output));
          strcat(output, query_string);
          output[strlen(query_string)] = alphabet[char_index];
          output[strlen(query_string) + 1] = '\0';

        } else if (button == 0) {
          memset(output, 0, sizeof(output));
          strcat(output, query_string);
          output[strlen(query_string)] = alphabet[char_index];
          output[strlen(query_string) + 1] = '\0';
        }

      } else if (state == 2) {
        memset(output, 0, sizeof(output));
        strcat(output, "Sending Query");
        state = 3;

//        TODO - modify to send gameID to server
        lookup(query_string, msg, 400);
        digitalWrite(R_PIN, HIGH);
        digitalWrite(G_PIN, HIGH);
        digitalWrite(B_PIN, LOW);
        prev_stonk_val = atof(msg);
        Serial.println("start1");
        Serial.println(prev_stonk_val);
        Serial.println(atof(msg));
        memset(output, 0, sizeof(output));
        strcat(output, query_string);
        strcat(output, ":  ");
        strcat(output, msg);
        timer = millis();
      } else if (state == 3) {
        if (button == 2) {
          state = 1;
          memset(query_string, 0, sizeof(query_string));
          strcat(query_string, "");
          digitalWrite(R_PIN, LOW);
          digitalWrite(B_PIN, LOW);
          digitalWrite(G_PIN, LOW);
        } else if (millis() - timer >= 10000) {
          timer = millis();
          lookup(query_string, msg, 400);
          Serial.println("start2");
          Serial.println(prev_stonk_val);
          Serial.println(atof(msg));
          if (prev_stonk_val < atof(msg) - 0.001) {
            Serial.println("less than");
            digitalWrite(R_PIN, LOW);
            digitalWrite(B_PIN, LOW);
            digitalWrite(G_PIN, HIGH);
          } else if (prev_stonk_val > atof(msg) + 0.001) {
            Serial.println("greater than");
            digitalWrite(G_PIN, LOW);
            digitalWrite(B_PIN, LOW);
            digitalWrite(R_PIN, HIGH);
          } else {
            Serial.println("equal");
            digitalWrite(R_PIN, HIGH);
            digitalWrite(G_PIN, HIGH);
            digitalWrite(B_PIN, LOW);
          }
          prev_stonk_val = atof(msg);
          memset(output, 0, sizeof(output));
          strcat(output, query_string);
          strcat(output, ":  ");
          strcat(output, msg);
        }
      }
    }
};

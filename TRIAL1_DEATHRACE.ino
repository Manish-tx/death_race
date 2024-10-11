#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <vector>
#include <iostream>
#include <sstream>

struct MOTOR_PINS
{
  int pinEn;  
  int pinIN1;
  int pinIN2;    
};

std::vector<MOTOR_PINS> motorPins = 
{
  {D1, D2, D3},  //RIGHT_MOTOR Pins (EnA, IN1, IN2)
  {D4, D5, D6},  //LEFT_MOTOR Pins (EnB, IN3, IN4)
};

#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define STOP 0

#define RIGHT_MOTOR 0
#define LEFT_MOTOR 1

#define FORWARD 1
#define BACKWARD -1

const char* ssid     = "MyWiFiCar";
const char* password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket wsCarInput("/CarInput");

const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<style>
.arrows {
  font-size:40px;
  color:red;
}
td.button {
  background-color:black;
  border-radius:25%;
  box-shadow: 5px 5px #888888;
}
td.button:active {
  transform: translate(5px,5px);
  box-shadow: none; 
}
.noselect {
  user-select: none;
}
.slidecontainer {
  width: 100%;
}
.slider {
  width: 100%;
  height: 20px;
  border-radius: 5px;
  background: #d3d3d3;
  outline: none;
  opacity: 0.7;
  transition: opacity .2s;
}
.slider:hover {
  opacity: 1;
}
.slider::-webkit-slider-thumb {
  width: 40px;
  height: 40px;
  border-radius: 50%;
  background: red;
  cursor: pointer;
}
.slider::-moz-range-thumb {
  width: 40px;
  height: 40px;
  border-radius: 50%;
  background: red;
  cursor: pointer;
}
</style>
</head>
<body class="noselect" align="center" style="background-color:white">
<h1 style="color: teal;text-align:center;">Hash Include Electronics</h1>
<h2 style="color: teal;text-align:center;">WiFi Tank Control</h2>
<table id="mainTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
  <tr>
    <td></td>
    <td class="button" ontouchstart='sendButtonInput("MoveCar","1")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">&#8679;</span></td>
    <td></td>
  </tr>
  <tr>
    <td class="button" ontouchstart='sendButtonInput("MoveCar","3")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">&#8678;</span></td>
    <td class="button"></td>    
    <td class="button" ontouchstart='sendButtonInput("MoveCar","4")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">&#8680;</span></td>
  </tr>
  <tr>
    <td></td>
    <td class="button" ontouchstart='sendButtonInput("MoveCar","2")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">&#8681;</span></td>
    <td></td>
  </tr>
  <tr/>
  <tr/>
  <tr/>
  <tr>
    <td style="text-align:left;font-size:25px"><b>Speed:</b></td>
    <td colspan=2>
     <div class="slidecontainer">
        <input type="range" min="0" max="255" value="150" class="slider" id="Speed" oninput='sendButtonInput("Speed",value)'>
      </div>
    </td>
  </tr>       
</table>
<script>
  var webSocketCarInputUrl = "ws://" + window.location.hostname + "/CarInput";      
  var websocketCarInput;
  
  function initCarInputWebSocket() 
  {
    websocketCarInput = new WebSocket(webSocketCarInputUrl);
    websocketCarInput.onopen = function(event)
    {
      var speedButton = document.getElementById("Speed");
      sendButtonInput("Speed", speedButton.value);
    };
    websocketCarInput.onclose = function(event){setTimeout(initCarInputWebSocket, 2000);};
    websocketCarInput.onmessage = function(event){};        
  }
  
  function sendButtonInput(key, value) 
  {
    var data = key + "," + value;
    websocketCarInput.send(data);
  }

  window.onload = initCarInputWebSocket;
  document.getElementById("mainTable").addEventListener("touchend", function(event){
    event.preventDefault()
  });      
</script>
</body>    
</html>
)HTMLHOMEPAGE";

void rotateMotor(int motorNumber, int motorDirection, int speed)
{
  if (motorDirection == FORWARD)
  {
    digitalWrite(motorPins[motorNumber].pinIN1, HIGH);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);    
  }
  else if (motorDirection == BACKWARD)
  {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, HIGH);     
  }
  else
  {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);       
  }
  
  analogWrite(motorPins[motorNumber].pinEn, speed);  // Set speed for the motor
}

void moveCar(int inputValue)
{
  Serial.printf("Got value as %d\n", inputValue);  
  switch(inputValue)
  {
    case UP:
      rotateMotor(RIGHT_MOTOR, FORWARD, 150); // Example speed
      rotateMotor(LEFT_MOTOR, FORWARD, 150);                   
      break;
  
    case DOWN:
      rotateMotor(RIGHT_MOTOR, BACKWARD, 150);
      rotateMotor(LEFT_MOTOR, BACKWARD, 150);  
      break;
  
    case LEFT:
      rotateMotor(RIGHT_MOTOR, FORWARD, 150);
      rotateMotor(LEFT_MOTOR, BACKWARD, 150);  
      break;
  
    case RIGHT:
      rotateMotor(RIGHT_MOTOR, BACKWARD, 150);
      rotateMotor(LEFT_MOTOR, FORWARD, 150); 
      break;
 
    case STOP:
      rotateMotor(RIGHT_MOTOR, STOP, 0);
      rotateMotor(LEFT_MOTOR, STOP, 0);    
      break;
  
    default:
      rotateMotor(RIGHT_MOTOR, STOP, 0);
      rotateMotor(LEFT_MOTOR, STOP, 0);    
      break;
  }
}

void handleRoot(AsyncWebServerRequest *request) 
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) 
{
  request->send(404, "text/plain", "File Not Found");
}

void onCarInputWebSocketEvent(AsyncWebSocket *server, 
                      AsyncWebSocketClient *client, 
                      AwsEventType type,
                      void *arg, 
                      uint8_t *data, 
                      size_t len) 
{                      
  switch (type) 
  {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().to


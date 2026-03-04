#ifndef ARDUINO_USB_MODE
#error This ESP32 SoC has no Native USB interface
#elif ARDUINO_USB_MODE == 1
#warning This sketch should be used when USB is in OTG mode
void setup() {}
void loop() {}
#else

#include <WiFi.h>
#include <NetworkClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
USBHIDKeyboard Keyboard;

const int buttonPin = 0;         // input pin for pushbutton
int previousButtonState = HIGH;  // for checking the state of a pushButton

const char *ssid = "OnePay";
const char *password = "2025@#2025";

WebServer server(80);

const int led = 13;

void printText() {
  // type out a message
  Keyboard.print("G84dh@#859Ac");
  // delay(100);
  Keyboard.press(KEY_RETURN);
  // delay(100);
  Keyboard.releaseAll();
}

void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/plain", "hello from esp32!");
  digitalWrite(led, 0);
}

void handleEnter() {
  printText();
  server.send(200, "text/plain", "password has been entered");
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.on("/enter", handleEnter);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  // make the pushButton pin an input:
  pinMode(buttonPin, INPUT_PULLUP);
  // initialize control over the keyboard:
  Keyboard.begin();
  USB.begin();
}

void loop(void) {
  server.handleClient();
  delay(2);  //allow the cpu to switch to other tasks

  // read the pushbutton:
  int buttonState = digitalRead(buttonPin);
  // if the button state has changed,
  if ((buttonState != previousButtonState)
      // and it's currently pressed:
      && (buttonState == LOW)) {
    printText();
  }
  // save the current button state for comparison next time:
  previousButtonState = buttonState;
}

#endif /* ARDUINO_USB_MODE */

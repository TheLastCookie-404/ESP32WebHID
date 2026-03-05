#ifndef ARDUINO_USB_MODE
#error This ESP32 SoC has no Native USB interface
#elif ARDUINO_USB_MODE == 1
#warning This sketch should be used when USB is in OTG mode
void setup() {}
void loop() {}
#else

#define AP_SSID "ESP32"
#define AP_PSWD "123123123"

#define FORMAT_LITTLEFS_IF_FAILED true

#include <WiFi.h>
#include <NetworkClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "LittleFS.h"
USBHIDKeyboard Keyboard;

const int buttonPin = 0;         // input pin for pushbutton
int previousButtonState = HIGH;  // for checking the state of a pushButton
int connectionTimeCounter = 0;

String ssid = "";
String password = "";

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

void handleSave() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");

  LittleFS.remove("/wifi.txt");
  File file = LittleFS.open("/wifi.txt", FILE_WRITE);

  if (ssid == "" && password == "") return;
  Serial.println(ssid);
  Serial.println(password);

  if (!file) return server.send(500, "text/plain", "cant save credentials!");
  file.println(ssid);
  file.println(password);

  server.send(200, "text/plain", "please enter credentials");
  ESP.restart();
}

void handleEnter() {
  // if (server.method() == HTTP_POST) {
  //   printText();
  //   server.send(200, "text/plain", "password has been entered");
  // } else
  // server.send(405, "text/plain", "http method mismatch");
  Serial.println(server.arg("lol"));
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

void readFile() {
  File file = LittleFS.open("/wifi.txt", FILE_READ);
  ssid = file.readStringUntil('\n');
  password = file.readStringUntil('\n');

  file.close();

  ssid.trim();
  password.trim();

  Serial.println(ssid);
  Serial.println(password);
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  delay(1000);

  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) { // Mount the filesystem
    Serial.println("Failed to mount LittleFS");
    ESP.restart();
  }

  Serial.println("LittleFS mounted!");

  readFile();

  if (ssid == "" || password == "") {
    WiFi.mode(WIFI_MODE_AP);
    WiFi.softAP(AP_SSID, AP_PSWD);
    server.on("/save", handleSave);
    server.begin();
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    delay(1000);
    Serial.println("");

    // Wait for connection
    while (WiFi.status()) {
      delay(500);
      Serial.print(".");

      connectionTimeCounter++;

      if (connectionTimeCounter > 20) ESP.restart();
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

    server.on("/disconnect", []() {
      server.send(200, "text/plain", "disconnecting");
      LittleFS.remove("/wifi.txt");
      ESP.restart();
    });

    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("HTTP server started");
  }

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

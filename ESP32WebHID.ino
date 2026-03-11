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

#define LED_PIN 48
#define NUM_PIXELS 1

#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <NetworkClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "LittleFS.h"

USBHIDKeyboard Keyboard;
Adafruit_NeoPixel pixels(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

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

  if (ssid == "" && password == "") 
    return server.send(200, "text/plain", "please enter credentials");
  Serial.println(ssid);
  Serial.println(password);

  if (!file) {
    pixels.setPixelColor(0, pixels.Color(150, 0, 0));
    pixels.show();
    return server.send(500, "text/plain", "cant save credentials!");
  }
  file.println(ssid);
  file.println(password);

  pixels.setPixelColor(0, pixels.Color(0, 150, 0));
  pixels.show();
  server.send(200, "text/plain", "credentials entered");

  Serial.println("Reload the device");
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
  pixels.begin();
  Serial.begin(115200);
  delay(1000);

  pixels.clear();
  pixels.show();

  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) { // Mount the filesystem
    Serial.println("Failed to mount LittleFS");
    delay(100);
    ESP.restart();
  }

  Serial.println("LittleFS mounted!");

  readFile();

  Serial.println(ssid);
  Serial.println(password);

  if (ssid == "" || password == "") {
    pixels.setPixelColor(0, pixels.Color(0, 0, 150));
    pixels.show();

    Serial.println("AP mode");
    WiFi.mode(WIFI_MODE_AP);
    WiFi.softAP(AP_SSID, AP_PSWD);
    server.on("/save", handleSave);
    server.begin();
  } else {
    Serial.println("Server mode");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");

      connectionTimeCounter++;

      if (connectionTimeCounter > 70) {
        LittleFS.remove("/wifi.txt");
        delay(100);
        ESP.restart();
      }
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
      delay(100);
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

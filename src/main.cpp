#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Mitsubishi.h>

extern "C"
{
  uint8_t temprature_sens_read();
}

const char *ssid = "***";
const char *password = "***";

#define ONE_WIRE_BUS D0            // Temperature probe is connected on pin D6
const uint16_t kIrLedSend = D6;    // GPIO pin to use. Recommended: 4 (D2)
const uint16_t kIrLedRecieve = D5; // GPIO pin to use. Recommended: 5 (D1)

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
IRMitsubishiAC ac(kIrLedSend);

WebServer server(80);

float externalTemperature = 0.0;
float internalTemperature = 0.0;
String lastIrCommand = "";
long rssi = 0;

void handleApiParameters()
{
  DynamicJsonDocument doc(1024);

  doc["externalTemperature"] = externalTemperature;
  doc["internalTemperature"] = internalTemperature;
  doc["rssi"] = rssi;
  doc["lastIrCommand"] = lastIrCommand;

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void handleApiISaveMode()
{
  lastIrCommand = "ISAVE 10C";
  ac.setISave10C(true);
  server.send(200, "text/plain", "OK");
}

void handleApiHeatCommand()
{
  String command = server.arg("temperature");
  lastIrCommand = "HEAT " + command + "C";
  ac.on();
  ac.setISave10C(false);
  ac.setMode(kMitsubishiAcHeat);
  ac.setTemp(command.toInt());
  server.send(200, "text/plain", "OK");
}

void handleApiOffCommand()
{
  lastIrCommand = "OFF";
  ac.off();
  server.send(200, "text/plain", "OK");
}

void handleHealthCheck()
{
  server.send(200, "text/plain", "OK");
}

void handleApiFiles()
{
  File root = SPIFFS.open("/");

  File file = root.openNextFile();

  DynamicJsonDocument doc(1024);

  while (file)
  {
    doc["files"].add(file.name());
    file = root.openNextFile();
  }
  file.close();
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

int getInternalTemperature()
{
  int temperature = 0;
  bool success = false;
  uint8_t raw = temprature_sens_read();
  Serial.print("Raw temperature value: ");
  Serial.println(raw);
  temperature = (raw - 32) / 1.8f;
  Serial.print("Calibrated temperature value: ");
  Serial.println(temperature);
  success = (raw != 128);
  if (success)
    return temperature;
  else
    return -1;
}

void setup()
{
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, 0); // Set blue LED off by default
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  sensors.begin();

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  digitalWrite(BUILTIN_LED, 1); // Set blue LED on when connected
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/healthcheck", handleHealthCheck);
  server.on("/files", handleApiFiles);
  server.on("/api/parameters", handleApiParameters);
  server.on("/api/command/heat", handleApiHeatCommand);
  server.on("/api/command/isave", handleApiISaveMode);
  server.on("/api/command/off", handleApiOffCommand);

  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
  }
  else
  {
    server.serveStatic("/", SPIFFS, "/index.html");
    server.serveStatic("/index.js", SPIFFS, "/index.js");
    server.serveStatic("/index.css", SPIFFS, "/index.css");
  }

  server.begin();
}

int loopCounter = 0;

void loop()
{
  if (loopCounter == 0)
  {
    sensors.requestTemperatures();
    float tempC = sensors.getTempCByIndex(0);
    // Check if reading was successful
    if (tempC != DEVICE_DISCONNECTED_C)
    {
      externalTemperature = tempC;
      Serial.print("Temperature for the device 1 (index 0) is: ");
      Serial.println(tempC);
    }
    else
    {
      Serial.println("Error: Could not read temperature data");
    }
  }

  if (loopCounter % 2 == 0)
  {
    int internalTemp = getInternalTemperature();
    if (internalTemp != -1)
      internalTemperature = internalTemp;
  }

  if (loopCounter % 5 == 0)
  {
    rssi = WiFi.RSSI();
  }
  server.handleClient();

  if (loopCounter > 10)
    loopCounter = 0;
  else
    loopCounter++;
}
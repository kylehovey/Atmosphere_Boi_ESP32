#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <SensirionI2CSen5x.h>
#include <Adafruit_SCD30.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define MAXBUF_REQUIREMENT 48

#if (defined(I2C_BUFFER_LENGTH) &&                 \
   (I2C_BUFFER_LENGTH >= MAXBUF_REQUIREMENT)) || \
  (defined(BUFFER_LENGTH) && BUFFER_LENGTH >= MAXBUF_REQUIREMENT)
#define USE_PRODUCT_INFO
#endif

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

const char* ssid = "***REMOVED***";
const char* password = "***REMOVED***";

const char* mqtt_server = "192.168.100.42";
const char* mqtt_state_topic = "home/sensor/atmosphereBoiV6/value";

WiFiClient espClient;
PubSubClient client(espClient);

IPAddress local_IP(192, 168, 100, 67);
IPAddress gateway(192, 168, 100, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(1, 1, 1, 1);
IPAddress secondaryDNS(1, 0, 0, 1);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SensirionI2CSen5x sen5x;
Adafruit_SCD30  scd30;

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "Atmosphere_Boi_V6";

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      return;
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      delay(5000);
    }
  }
}

void drawText(const String& text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(true);
  display.setCursor(0, 0);
  display.cp437(true); 
  display.print(text);

  display.display();
}

void setup() {
  Serial.begin(112500);

  Serial.println("Atmosphere Boi: v6");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    while (1) { delay(10); }
  }

  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);

  if (!client.connected()) {
    reconnect();
  }

  Wire.begin();

  String discoverPm = "{\"name\":\"Atmosphere Boi V6 PM2.5\",\"stat_t\":\"home/sensor/atmosphereBoiV6/value\",\"unit_of_meas\":\"µg/m³\",\"dev_cla\":\"pm25\",\"frc_upd\":true,\"val_tpl\":\"{{ value_json.pm25|default(0) }}\"}";
  String discoverHu = "{\"name\":\"Atmosphere Boi V6 Humidity\",\"stat_t\":\"home/sensor/atmosphereBoiV6/value\",\"unit_of_meas\":\"%\",\"dev_cla\":\"humidity\",\"frc_upd\":true,\"val_tpl\":\"{{ value_json.humidity|default(0) }}\"}";
  String discoverTe = "{\"name\":\"Atmosphere Boi V6 Temperature\",\"stat_t\":\"home/sensor/atmosphereBoiV6/value\",\"unit_of_meas\":\"˚F\",\"dev_cla\":\"temperature\",\"frc_upd\":true,\"val_tpl\":\"{{ value_json.temperature|default(0) }}\"}";
  String discoverVO = "{\"name\":\"Atmosphere Boi V6 VOC Index\",\"stat_t\":\"home/sensor/atmosphereBoiV6/value\",\"dev_cla\":\"aqi\",\"frc_upd\":true,\"val_tpl\":\"{{ value_json.voc|default(0) }}\"}";
  String discoverNO = "{\"name\":\"Atmosphere Boi V6 NOX Index\",\"stat_t\":\"home/sensor/atmosphereBoiV6/value\",\"dev_cla\":\"aqi\",\"frc_upd\":true,\"val_tpl\":\"{{ value_json.nox|default(0) }}\"}";
  String discoverCO = "{\"name\":\"Atmosphere Boi V6 CO2\",\"stat_t\":\"home/sensor/atmosphereBoiV6/value\",\"unit_of_meas\":\"ppm\",\"dev_cla\":\"carbon_dioxide\",\"frc_upd\":true,\"val_tpl\":\"{{ value_json.co2|default(0) }}\"}";

  client.publish("homeassistant/sensor/atmosphereBoiV6_pm25/config", discoverPm.c_str(), true);
  client.publish("homeassistant/sensor/atmosphereBoiV6_humidity/config", discoverHu.c_str(), true);
  client.publish("homeassistant/sensor/atmosphereBoiV6_temp/config", discoverTe.c_str(), true);
  client.publish("homeassistant/sensor/atmosphereBoiV6_voc/config", discoverVO.c_str(), true);
  client.publish("homeassistant/sensor/atmosphereBoiV6_nox/config", discoverNO.c_str(), true);
  client.publish("homeassistant/sensor/atmosphereBoiV6_co2/config", discoverCO.c_str(), true);

  Serial.println("Initializing SCD30");
  if (!scd30.begin()) {
    Serial.println("Failed to find SCD30 chip");
    while (1) { delay(10); }
  }

  Serial.println("Starting Display");
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("Failed to initialize display");
    while (1) { delay(10); }
  }

  display.clearDisplay();
  display.display();

  Serial.println("Initializing SEN55");
  sen5x.begin(Wire);

  uint16_t error;
  char errorMessage[256];
  error = sen5x.deviceReset();

  if (error) {
    Serial.print("Error trying to execute deviceReset(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    while (1) { delay(10); }
  }

  float tempOffset = 0.0;
  error = sen5x.setTemperatureOffsetSimple(tempOffset);

  if (error) {
    Serial.print("Error trying to execute setTemperatureOffsetSimple(): ");
    Serial.println(errorMessage);
    while (1) { delay(10); }
  }

  error = sen5x.startMeasurement();

  if (error) {
    Serial.print("Error trying to execute startMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    while (1) { delay(10); }
  }

  drawText("Okay!");
}

float massConcentrationPm1p0;
float massConcentrationPm2p5;
float massConcentrationPm4p0;
float massConcentrationPm10p0;
float ambientHumidity;
float ambientTemperature;
float vocIndex;
float noxIndex;
float co2;

void loop() {
  uint16_t error;
  char errorMessage[256];

  delay(1000);

  error = sen5x.readMeasuredValues(
    massConcentrationPm1p0,
    massConcentrationPm2p5,
    massConcentrationPm4p0,
    massConcentrationPm10p0,
    ambientHumidity,
    ambientTemperature,
    vocIndex,
    noxIndex
  );

  if (error) {
    Serial.print("Error trying to execute readMeasuredValues(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);

    return;
  }

  if (scd30.dataReady()) {
    if (!scd30.read()){
      Serial.println("Error reading sensor data");
    }

    co2 = scd30.CO2;
  }

  bool allStatsReady = true;

  Serial.print("MassConcentrationPm2p5:");
  Serial.print(massConcentrationPm2p5);

  if (!isnan(ambientHumidity)) {
    Serial.print("AmbientHumidity:");
    Serial.println(ambientHumidity);
  } else {
    allStatsReady = false;
  }

  if (!isnan(ambientTemperature)) {
    Serial.print("AmbientTemperature:");
    Serial.println(ambientTemperature);
  } else {
    allStatsReady = false;
  }

  if (!isnan(vocIndex)) {
    Serial.print("VocIndex:");
    Serial.println(vocIndex);
  } else {
    allStatsReady = false;
  }

  if (!isnan(noxIndex)) {
    Serial.print("NoxIndex:");
    Serial.println(noxIndex);
  } else {
    allStatsReady = false;
  }

  if (!isnan(co2)) {
    Serial.print("CO2: ");
    Serial.print(co2, 3);
    Serial.println(" ppm");
  } else {
    allStatsReady = false;
  }

  if (allStatsReady) {
    Serial.println("Posting stats.");
    auto state = String("{\"humidity\":") + String(ambientHumidity, 2) + String(",");
    state += String("\"temperature\":") + String(ambientTemperature * 9/5 + 32, 2) + String(",");
    state += String("\"voc\":") + String(vocIndex, 2) + String(",");
    state += String("\"nox\":") + String(noxIndex, 2) + String(",");
    state += String("\"co2\":") + String(co2, 2) + String(",");
    state += String("\"pm25\":") + String(massConcentrationPm2p5, 2) + String("}");

    Serial.println("Publishing the following:");
    Serial.println(state);

    client.publish(mqtt_state_topic, state.c_str());
  }

  display.clearDisplay();

  display.setCursor(0, 0);

  display.println("Atmosphere Boi V6");

  display.print("PM2.5: ");
  display.print(massConcentrationPm2p5, 2);
  display.println("ug/m3");

  display.print("Humidity: ");
  display.print(ambientHumidity, 2);
  display.println("%");

  display.print("Temperature: ");
  display.print(ambientTemperature, 2);
  display.println(" C");

  display.print("VOC Index: ");
  display.print(vocIndex, 2);
  display.println("");

  display.print("NOX Index: ");
  display.print(noxIndex, 2);
  display.println("");

  display.print("CO2: ");
  display.print(co2, 2);
  display.println("ppm");

  display.display();

  client.loop();
}

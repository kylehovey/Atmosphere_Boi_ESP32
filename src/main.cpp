#include <Arduino.h>
#include <SensirionI2CSen5x.h>
#include <Adafruit_SCD30.h>
#include <Wire.h>

#define MAXBUF_REQUIREMENT 48

#if (defined(I2C_BUFFER_LENGTH) &&                 \
   (I2C_BUFFER_LENGTH >= MAXBUF_REQUIREMENT)) || \
  (defined(BUFFER_LENGTH) && BUFFER_LENGTH >= MAXBUF_REQUIREMENT)
#define USE_PRODUCT_INFO
#endif

SensirionI2CSen5x sen5x;
Adafruit_SCD30  scd30;

void setup() {
  Serial.begin(112500);

  while (!Serial) {
    delay(100);
  }

  Wire.begin();

  sen5x.begin(Wire);

  if (!scd30.begin()) {
    Serial.println("Failed to find SCD30 chip");
    while (1) { delay(10); }
  }

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
}

void loop() {
  uint16_t error;
  char errorMessage[256];

  delay(1000);

  // Read Measurement
  float massConcentrationPm1p0;
  float massConcentrationPm2p5;
  float massConcentrationPm4p0;
  float massConcentrationPm10p0;
  float ambientHumidity;
  float ambientTemperature;
  float vocIndex;
  float noxIndex;

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

  Serial.print("MassConcentrationPm2p5:");
  Serial.print(massConcentrationPm2p5);
  Serial.print("\t");

  if (!isnan(ambientHumidity)) {
    Serial.print("AmbientHumidity:");
    Serial.print(ambientHumidity);
    Serial.print("\t");
  }

  if (!isnan(ambientTemperature)) {
    Serial.print("AmbientTemperature:");
    Serial.print(ambientTemperature);
    Serial.print("\t");
  }

  if (!isnan(vocIndex)) {
    Serial.print("VocIndex:");
    Serial.print(vocIndex);
    Serial.print("\t");
  }

  if (!isnan(noxIndex)) {
    Serial.print("NoxIndex:");
    Serial.println(noxIndex);
  }

  if (scd30.dataReady()) {
    if (!scd30.read()){
      Serial.print("Error reading sensor data");
    }

    Serial.print("CO2: ");
    Serial.print(scd30.CO2, 3);
    Serial.println(" ppm");
  }
}

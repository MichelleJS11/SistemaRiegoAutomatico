#include <SoftwareSerial.h>
#include <ArduinoJson.h>

// Initialize a SoftwareSerial object to communicate with the HC-05
SoftwareSerial bluetooth(10, 11);  // RX | TX
JsonDocument SensorsData;

//Variables para guardar los pines designados para los componentes
const byte ledPin = 3;
const byte pumpPin = 4;
const byte potentiometerPin = A3;
const byte lightSensor = A4;
const byte humiditySensorAnalogPin = A5;

int AnalogValueMin = 0;
int AnalogValueMax = 1023;

//Variable para guardar valores leidos por el sensor de humedad, el potenciometro y el sensor de luz
int currentHumidity = 0;
int currentHumidityThreshold = 0;
boolean currentDayTime = false;

boolean isPumpOn = false;
int dryHysteresis = 3;
unsigned long previousCheckTime = 0;
const int8_t checkInterval = 100;

struct Data {
  const String name;
  const String value;
};

void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);
  pinMode(ledPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
}

void loop() {
  updateSensorValues();
  maintainSoilHumidity();
  sendDataOverBluetooth();
  delay(100);
}

void updateSensorValues() {
  currentHumidity = getHumidity();
  currentHumidityThreshold = getHumidityThreshold();
  currentDayTime = getDayTime();
}

void maintainSoilHumidity() {
  unsigned long currentTime = millis();

  if (currentTime - previousCheckTime < checkInterval) return;
  else previousCheckTime = currentTime;

  if (currentHumidity < currentHumidityThreshold && !isPumpOn) {
    digitalWrite(pumpPin, HIGH);
    digitalWrite(ledPin, LOW);
    isPumpOn = true;
  } else if (currentHumidity > currentHumidityThreshold + dryHysteresis && isPumpOn) {
    digitalWrite(pumpPin, LOW);
    digitalWrite(ledPin, HIGH);
    isPumpOn = false;
  }
}

void sendDataOverBluetooth() {
  Data data[] = {
    { "currentDayTime", String(currentDayTime) },
    { "currentHumidity", String(currentHumidity) },
    { "currentHumidityThreshold", String(currentHumidityThreshold) },
    { "isPumpWorking", String(isPumpOn) }
  };
  const int arraySize = sizeof(data) / sizeof(data[0]);
  serializeJson(createJsonObject(data, arraySize), bluetooth);
  bluetooth.println();
}

int getHumidity() {
  int humidity = map(analogRead(humiditySensorAnalogPin), AnalogValueMax, AnalogValueMin, 0, 100);
  return constrain(humidity, 0, 100);
}

int getHumidityThreshold() {
  int humidityThreshold = map(analogRead(potentiometerPin), AnalogValueMin, AnalogValueMax, 0, 100);
  return constrain(humidityThreshold, 0, 100);
}

boolean getDayTime() {
  if (analogRead(lightSensor) > 150) {
    return true;
  }
  return false;
}

JsonDocument createJsonObject(Data data[], int lenght) {
  JsonDocument newSensorData;
  for (int i = 0; i < lenght; i++) {
    newSensorData[data[i].name] = data[i].value;
  }
  return newSensorData;
}
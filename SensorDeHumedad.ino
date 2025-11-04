#include <SoftwareSerial.h>   // Librería para crear una comunicación serial por pines digitales
#include <ArduinoJson.h>       // Librería para manejar datos en formato JSON

// Se crea una conexión serial por software para el módulo Bluetooth en los pines 10 (RX) y 11 (TX)
SoftwareSerial bluetooth(10, 11);

// Objeto JSON para almacenar los datos de los sensores
JsonDocument SensorsData;

// Definición de los pines utilizados
const byte ledPin = 3;                // Pin del LED indicador
const byte pumpPin = 4;               // Pin que controla la bomba de agua
const byte potentiometerPin = A3;     // Pin del potenciómetro (control de umbral de humedad)
const byte lightSensor = A4;          // Pin del sensor de luz (para detectar día/noche)
const byte humiditySensorAnalogPin = A5; // Pin analógico del sensor de humedad del suelo

// Valores mínimos y máximos del rango analógico
int AnalogValueMin = 0;
int AnalogValueMax = 1023;

// Variables para guardar las lecturas de los sensores
int currentHumidity = 0;
int currentHumidityThreshold = 0;
boolean currentDayTime = false;

// Variables de estado
boolean isPumpOn = false;             // Indica si la bomba está encendida
int dryHysteresis = 3;                // Margen para evitar encendidos/apagados rápidos
unsigned long previousCheckTime = 0;  // Tiempo de la última comprobación de humedad
const int8_t checkInterval = 100;     // Intervalo entre comprobaciones (en milisegundos)

// Estructura para almacenar pares nombre-valor que se enviarán en el JSON
struct Data {
  const String name;
  const String value;
};

void setup() {
  Serial.begin(9600);        // Inicializa la comunicación con el monitor serial
  bluetooth.begin(9600);     // Inicializa la comunicación Bluetooth
  pinMode(ledPin, OUTPUT);   // Configura el pin del LED como salida
  pinMode(pumpPin, OUTPUT);  // Configura el pin de la bomba como salida
}

void loop() {
  updateSensorValues();       // Actualiza las lecturas de los sensores
  maintainSoilHumidity();     // Controla la bomba según la humedad del suelo
  sendDataOverBluetooth();    // Envía los datos por Bluetooth
  delay(100);                 // Pequeña pausa para estabilidad
}

// Función para actualizar los valores de todos los sensores
void updateSensorValues() {
  currentHumidity = getHumidity();                 // Lee la humedad del suelo
  currentHumidityThreshold = getHumidityThreshold(); // Lee el umbral de humedad definido por el potenciómetro
  currentDayTime = getDayTime();                   // Determina si es de día o de noche
}

// Controla el encendido y apagado de la bomba según la humedad del suelo
void maintainSoilHumidity() {
  unsigned long currentTime = millis(); // Tiempo actual en milisegundos

  // Solo se realiza la comprobación cada cierto intervalo
  if (currentTime - previousCheckTime < checkInterval) return;
  else previousCheckTime = currentTime;

  // Si la humedad actual es menor que el umbral y la bomba está apagada → encender bomba
  if (currentHumidity < currentHumidityThreshold && !isPumpOn) {
    digitalWrite(pumpPin, HIGH);  // Activa la bomba
    digitalWrite(ledPin, LOW);    // Apaga el LED (indicador opcional)
    isPumpOn = true;
  } 
  // Si la humedad supera el umbral + histéresis y la bomba está encendida → apagar bomba
  else if (currentHumidity > currentHumidityThreshold + dryHysteresis && isPumpOn) {
    digitalWrite(pumpPin, LOW);   // Apaga la bomba
    digitalWrite(ledPin, HIGH);   // Enciende el LED
    isPumpOn = false;
  }
}

// Envía los datos de sensores por Bluetooth en formato JSON
void sendDataOverBluetooth() {
  Data data[] = {
    { "currentDayTime", String(currentDayTime) },
    { "currentHumidity", String(currentHumidity) },
    { "currentHumidityThreshold", String(currentHumidityThreshold) },
    { "isPumpWorking", String(isPumpOn) }
  };

  // Calcula cuántos elementos hay en el Array
  const int arraySize = sizeof(data) / sizeof(data[0]);

  // Convierte los datos a formato JSON y los envía por Bluetooth
  serializeJson(createJsonObject(data, arraySize), bluetooth);
  bluetooth.println();  // Salto de línea para mejorar la lectura
}

// Lee el valor del sensor de humedad del suelo y lo convierte a porcentaje (0–100%)
int getHumidity() {
  int humidity = map(analogRead(humiditySensorAnalogPin), AnalogValueMax, AnalogValueMin, 0, 100);
  return constrain(humidity, 0, 100); // Asegura que el valor esté entre 0 y 100
}

// Lee el valor del potenciómetro y lo convierte en un umbral de humedad (0–100%)
int getHumidityThreshold() {
  int humidityThreshold = map(analogRead(potentiometerPin), AnalogValueMin, AnalogValueMax, 0, 100);
  return constrain(humidityThreshold, 0, 100);
}

// Determina si actualmente es de día o de noche según la lectura del sensor de luz
boolean getDayTime() {
  if (analogRead(lightSensor) > 150) { // 150 es el valor límite de brillo
    return true;  // Día
  }
  return false;   // Noche
}

// Crea un objeto JSON a partir de un arreglo de estructuras Data
JsonDocument createJsonObject(Data data[], int lenght) {
  JsonDocument newSensorData;
  for (int i = 0; i < lenght; i++) {
    newSensorData[data[i].name] = data[i].value; // Añade cada par nombre-valor al JSON
  }
  return newSensorData;
}

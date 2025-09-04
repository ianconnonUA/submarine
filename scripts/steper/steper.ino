/* 
Ejemplo adaptado para ESP32-S
Control de motor paso a paso 28BYJ48 sin librería
Basado en Luis Llamas
*/

// Definición de pines en ESP32-S (usar GPIO válidos)
const int motorPin1 = 16;  // 28BYJ48 In1
const int motorPin2 = 17;  // 28BYJ48 In2
const int motorPin3 = 18;  // 28BYJ48 In3
const int motorPin4 = 19;  // 28BYJ48 In4

// Variables
int motorSpeed = 1200;   // Velocidad en microsegundos
int stepCounter = 0;     // Contador de pasos
int stepsPerRev = 4076;  // Pasos por vuelta completa (28BYJ48 con reductor)

// Secuencia media fase (más suave)
const int numSteps = 8;
const int stepsLookup[8] = {
  B1000, B1100, B0100, B0110,
  B0010, B0011, B0001, B1001
};

void setup() {
  // Declarar pines como salida
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);
}

void loop() {
  // Giro horario
  for (int i = 0; i < stepsPerRev; i++) {
    clockwise();
    delayMicroseconds(motorSpeed);
  }

  delay(1000);

  // Giro antihorario
  for (int i = 0; i < stepsPerRev; i++) {
    anticlockwise();
    delayMicroseconds(motorSpeed);
  }

  delay(1000);
}

void clockwise() {
  stepCounter++;
  if (stepCounter >= numSteps) stepCounter = 0;
  setOutput(stepCounter);
}

void anticlockwise() {
  stepCounter--;
  if (stepCounter < 0) stepCounter = numSteps - 1;
  setOutput(stepCounter);
}

void setOutput(int step) {
  digitalWrite(motorPin1, bitRead(stepsLookup[step], 0));
  digitalWrite(motorPin2, bitRead(stepsLookup[step], 1));
  digitalWrite(motorPin3, bitRead(stepsLookup[step], 2));
  digitalWrite(motorPin4, bitRead(stepsLookup[step], 3));
}
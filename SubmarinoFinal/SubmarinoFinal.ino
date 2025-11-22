#include <WiFi.h>
#include <WebServer.h>
// #include <ESP32Servo.h> // Eliminado
#include <OneWire.h>
#include <DallasTemperature.h>

// ========================
// CONFIG WIFI (MODO AP)
// ========================
const char* ssid = "SubmarinoESP32";
const char* password = "12345678";

// ========================
// PINES
// ========================
// const int servoPin = 23; // Eliminado
// Motor A (Llenar tanque)
const int motorA_in1 = 27;
const int motorA_in2 = 26;
const int motorA_ena = 12;
// Motor B (Vaciar tanque)
const int motorB_in1 = 25;
const int motorB_in2 = 32;
const int motorB_enb = 33;
// Relay JT800 (propulsión)
const int relayPin = 13;
// Sensor DS18B20
const int oneWireBus = 15;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

// ========================
// OBJETOS GLOBALES
// ========================
WebServer server(80);
// Servo direccionServo; // Eliminado

// ========================
// VARIABLES DE ESTADO (PARA LÓGICA NO-BLOQUEANTE)
// ========================
enum SystemState { STATE_IDLE, STATE_FILLING, STATE_EMPTYING, STATE_PROPULSION };
SystemState currentState = STATE_IDLE;
unsigned long actionStartTime = 0; // Marca de tiempo (millis) de cuándo empezó una acción

// Tiempos de operación (en milisegundos)
const unsigned long FILL_DURATION = 60000;  // 1 minuto
const unsigned long EMPTY_DURATION = 70000; // 1 minuto 10 segundos
// const unsigned long PROPULSION_DURATION = 8000; // 8 segundos

// --- Variables para el temporizador del Servo --- // Eliminadas


// -----------------------------------------------------------------
// 1. PLANTILLA HTML EN MEMORIA FLASH (PROGMEM)
// (HTML Modificado sin la sección del servo)
// -----------------------------------------------------------------
const char html_PROGMEM[] PROGMEM = R"RAW(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Submarine</title>
    <style>
        body { font-family: Arial, sans-serif; background-color: #f4f4f4; color: #333; max-width: 600px; margin: 0 auto; padding: 20px; text-align: center; }
        h1 { color: #0056b3; }
        h2 { border-bottom: 2px solid #ccc; padding-bottom: 5px; margin-top: 30px; }
        .button-group { display: flex; justify-content: center; gap: 15px; margin-top: 15px; }
        .button { display: inline-block; padding: 12px 20px; font-size: 16px; font-weight: bold; color: #fff; background-color: #007bff; border: none; border-radius: 5px; text-decoration: none; cursor: pointer; transition: background-color 0.3s; }
        .button:hover { background-color: #0056b3; }
        .button-fill { background-color: #28a745; }
        .button-fill:hover { background-color: #218838; }
        .button-empty { background-color: #ffc107; }
        .button-empty:hover { background-color: #e0a800; }
        p { font-size: 1.2em; margin-top: 20px; }
        .footer { font-size:12px; color:gray; margin-top: 30px; }
        .button-stop { background-color: #dc3545; width: 90%; padding: 20px; font-size: 1.5em; margin-top: 20px; border: 3px solid #b02a37; }
        .button-stop:hover { background-color: #c82333; }
        .status-box { background-color: #e9ecef; border: 1px solid #ced4da; border-radius: 5px; padding: 15px; margin-top: 20px; }
        .status-box p { margin: 0; }
        .status-box b { color: #0056b3; }
    </style>
</head>
<body>
    <h1>Control Submarino</h1>
    <a class="button button-stop" href="/stop">PARADA DE EMERGENCIA</a>
    <div class="status-box">
        <p><b>Estado:</b> %%STATUS%%</p>
    </div>

    <h2>Tanque de Lastre</h2>
    <div class="button-group">
        <a class="button button-fill" href="/tanque/llenar">Llenar (60s)</a>
        <a class="button button-empty" href="/tanque/vaciar">Vaciar (70s)</a>
    </div>

    <h2>Propulsión (Manual)</h2>
    <div class="button-group">
        <a class="button button-fill" href="/propulsar/avanzar">Avanzar</a>
        <a class="button button-empty" href="/propulsar/parar">Parar</a>
    </div>

    <h2>Telemetría</h2>
    <p><b>Temperatura:</b> %%TEMP%%</p>
    <p class="footer">
        Conéctate a la red WiFi <b>%%SSID%%</b> y abrí <b>http://192.168.4.1</b>
    </p>
    <script>
        setTimeout(function(){
            window.location.reload(1);
        }, 5000); // Refresca cada 5 segundos
    </script>
</body>
</html>
)RAW";


// ========================
// FUNCIONES DE PARADA
// (Sin cambios)
// ========================
void stopAllActions() {
  Serial.println("PARADA DE EMERGENCIA o Fin de Tarea.");
  analogWrite(motorA_ena, 0);
  digitalWrite(motorA_in1, LOW);
  digitalWrite(motorA_in2, LOW);
  analogWrite(motorB_enb, 0);
  digitalWrite(motorB_in1, LOW);
  digitalWrite(motorB_in2, LOW);
  digitalWrite(relayPin, LOW);
  currentState = STATE_IDLE;
  actionStartTime = 0;
}

// ========================
// MANEJADORES HTML
// (handleRoot sin cambios)
// ========================
void handleRoot() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  String html = FPSTR(html_PROGMEM);
  String tempStr;
  if (tempC == DEVICE_DISCONNECTED_C) {
    tempStr = "Sensor no detectado";
  } else {
    tempStr = String(tempC, 2) + " &deg;C";
  }
  String statusStr = "En espera";
  unsigned long elapsed = (millis() - actionStartTime) / 1000;
  switch (currentState) {
    case STATE_FILLING:
      statusStr = "Llenando tanque... (" + String(elapsed) + "s / 60s)";
      break;
    case STATE_EMPTYING:
      statusStr = "Vaciando tanque... (" + String(elapsed) + "s / 70s)";
      break;
    case STATE_PROPULSION:
      statusStr = "Propulsando...";
      break;
    case STATE_IDLE:
    default:
      statusStr = "En espera";
      break;
  }
  html.replace("%%TEMP%%", tempStr);
  html.replace("%%SSID%%", ssid);
  html.replace("%%STATUS%%", statusStr);
  server.send(200, "text/html", html);
}

// (handleStop sin cambios)
void handleStop() {
  stopAllActions();
  server.sendHeader("Location", "/");
  server.send(303);
}

// ========================
// CONTROL DE SERVO (¡ELIMINADO!)
// ========================
// void handleServo(String dir) { ... } // Eliminado


// ========================
// CONTROL DE MOTORES (Sin cambios)
// ========================
void handleTanque(String accion) {
  if (currentState != STATE_IDLE) {
    server.sendHeader("Location", "/");
    server.send(303);
    return;
  }
  if (accion == "llenar") {
    Serial.println("Iniciando llenado de tanque...");
    digitalWrite(motorA_in1, HIGH);
    digitalWrite(motorA_in2, LOW);
    analogWrite(motorA_ena, 200);
    currentState = STATE_FILLING;
    actionStartTime = millis();
  }
  if (accion == "vaciar") {
    Serial.println("Iniciando vaciado de tanque...");
    digitalWrite(motorB_in1, HIGH);
    digitalWrite(motorB_in2, LOW);
    analogWrite(motorB_enb, 200);
    currentState = STATE_EMPTYING;
    actionStartTime = millis();
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

// ========================
// CONTROL DE JT800 (Sin cambios)
// ========================
void handlePropulsion(String accion) {
  
  // --- INICIAR PROPULSIÓN ---
  if (accion == "avanzar") {
    // Solo arranca si el sistema está en espera
    if (currentState != STATE_IDLE) {
      server.sendHeader("Location", "/");
      server.send(303);
      return; 
    }
    Serial.println("Iniciando propulsión (manual)...");
    digitalWrite(relayPin, HIGH);
    currentState = STATE_PROPULSION; // Marcar estado como "Propulsando"
  }
  
  // --- DETENER PROPULSIÓN ---
  if (accion == "parar") {
    // Solo intenta parar si el estado actual es "Propulsando"
    if (currentState == STATE_PROPULSION) { 
      Serial.println("Propulsión (manual) detenida.");
      digitalWrite(relayPin, LOW);
      currentState = STATE_IDLE; // Marcar estado como "En espera"
    }
    // Si se presiona "Parar" y el estado es OTRO (ej. Llenando), no hace nada.
  }
  
  server.sendHeader("Location", "/");
  server.send(303);
}


// ========================
// SETUP (¡MODIFICADO!)
// ========================
void setup() {
  Serial.begin(115200);
  Serial.println("\nIniciando Access Point...");

  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("Red WiFi creada: "); Serial.println(ssid);
  Serial.print("IP del submarino: "); Serial.println(myIP);

  // Motores
  pinMode(motorA_in1, OUTPUT); pinMode(motorA_in2, OUTPUT); pinMode(motorA_ena, OUTPUT);
  pinMode(motorB_in1, OUTPUT); pinMode(motorB_in2, OUTPUT); pinMode(motorB_enb, OUTPUT);
  digitalWrite(motorA_in1, LOW); digitalWrite(motorA_in2, LOW);
  digitalWrite(motorB_in1, LOW); digitalWrite(motorB_in2, LOW);

  // Relay
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Sensor
  sensors.begin();

  // Servidor web
  server.on("/", handleRoot);
  server.on("/stop", handleStop);
  
  // Rutas del servo eliminadas
  // server.on("/servo/izquierda", []() { handleServo("izquierda"); });
  // server.on("/servo/centro", []() { handleServo("centro"); });
  // server.on("/servo/derecha", []() { handleServo("derecha"); });

  server.on("/tanque/llenar", []() { handleTanque("llenar"); });
  server.on("/tanque/vaciar", []() { handleTanque("vaciar"); });

  server.on("/propulsar/avanzar", []() { handlePropulsion("avanzar"); });
  server.on("/propulsar/parar", []() { handlePropulsion("parar"); });

  server.begin();
  Serial.println("Servidor web iniciado.");
}

// ========================
// LOOP (¡MODIFICADO!)
// ========================
void loop() {
  // 1. Siempre atender al servidor web
  server.handleClient();

  // 2. Gestionar los estados de las acciones (Tanques, Propulsión)
  if (currentState != STATE_IDLE) {
    unsigned long currentTime = millis();

    if (currentState == STATE_FILLING) {
      if (currentTime - actionStartTime >= FILL_DURATION) {
        Serial.println("Llenado de tanque completado.");
        stopAllActions();
      }
    }
    else if (currentState == STATE_EMPTYING) {
      if (currentTime - actionStartTime >= EMPTY_DURATION) {
        Serial.println("Vaciado de tanque completado.");
        stopAllActions();
      }
    }
  }

  // 3. Gestionar el temporizador de "detach" del servo (ELIMINADO)
  // if (isServoActive) { ... } // Eliminado
  
}
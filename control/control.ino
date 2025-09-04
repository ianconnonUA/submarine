#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <AccelStepper.h>

// ========================
// CONFIG WIFI (MODO AP)
// ========================
const char* ssid = "SubmarinoESP32";   // Nombre de la red que creará el ESP32
const char* password = "12345678";     // Contraseña (mínimo 8 caracteres)

// ========================
// PINES
// ========================
const int servoPin = 13;
Servo direccionServo;

const int bombaPin1 = 26;
const int bombaPin2 = 27;
const int bombaENA = 12;

const int IN1_PIN = 32;
const int IN2_PIN = 33;
const int IN3_PIN = 25;
const int IN4_PIN = 14;

AccelStepper stepper(AccelStepper::FULL4WIRE, IN1_PIN, IN3_PIN, IN2_PIN, IN4_PIN);

// ========================
// SERVIDOR WEB
// ========================
WebServer server(80);

// ========================
// MANEJADORES
// ========================
void handleRoot() {
  String html = "<html><body><h1>Control Submarino</h1>";
  html += "<h2>Direccion (Servo)</h2>";
  html += "<a href=\"/servo/izquierda\"><button>Izquierda</button></a>";
  html += "<a href=\"/servo/centro\"><button>Centro</button></a>";
  html += "<a href=\"/servo/derecha\"><button>Derecha</button></a>";

  html += "<h2>Propulsion (Bomba)</h2>";
  html += "<a href=\"/bomba/adelante\"><button>Adelante</button></a>";
  html += "<a href=\"/bomba/parar\"><button>Parar</button></a>";

  html += "<h2>Inmersion (Stepper)</h2>";
  html += "<a href=\"/stepper/llenar\"><button>Llenar Jeringa</button></a>";
  html += "<a href=\"/stepper/vaciar\"><button>Vaciar Jeringa</button></a>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleServo(String dir) {
  if (dir == "izquierda") direccionServo.write(45);
  if (dir == "centro") direccionServo.write(90);
  if (dir == "derecha") direccionServo.write(135);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleBomba(String accion) {
  if (accion == "adelante") {
    digitalWrite(bombaPin1, HIGH);
    digitalWrite(bombaPin2, LOW);
    analogWrite(bombaENA, 250);
  }
  if (accion == "parar") {
    digitalWrite(bombaPin1, LOW);
    digitalWrite(bombaPin2, LOW);
    analogWrite(bombaENA, 0);
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleStepper(String accion) {
  if (accion == "llenar") {
    stepper.moveTo(stepper.currentPosition() + 4048);
  }
  if (accion == "vaciar") {
    stepper.moveTo(stepper.currentPosition() - 4048);
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

// ========================
// SETUP
// ========================
void setup() {
  Serial.begin(115200);
  Serial.println();

  // Iniciar WiFi en modo Access Point
  Serial.println("Iniciando Access Point...");
  WiFi.softAP(ssid, password);

  // Obtener IP asignada al AP
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("Red WiFi creada: ");
  Serial.println(ssid);
  Serial.print("IP del submarino (AP): ");
  Serial.println(myIP);

  // Config Servo
  direccionServo.attach(servoPin);
  direccionServo.write(90);

  // Config Bomba
  pinMode(bombaPin1, OUTPUT);
  pinMode(bombaPin2, OUTPUT);
  digitalWrite(bombaPin1, LOW);
  digitalWrite(bombaPin2, LOW);

  // Config Stepper
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);
  stepper.setCurrentPosition(0);

  // Config servidor web
  server.on("/", handleRoot);
  server.on("/servo/izquierda", []() { handleServo("izquierda"); });
  server.on("/servo/centro", []() { handleServo("centro"); });
  server.on("/servo/derecha", []() { handleServo("derecha"); });

  server.on("/bomba/adelante", []() { handleBomba("adelante"); });
  server.on("/bomba/parar", []() { handleBomba("parar"); });

  server.on("/stepper/llenar", []() { handleStepper("llenar"); });
  server.on("/stepper/vaciar", []() { handleStepper("vaciar"); });

  server.begin();
  Serial.println("Servidor web iniciado.");
}

// ========================
// LOOP
// ========================
void loop() {
  server.handleClient();
  stepper.run();
}

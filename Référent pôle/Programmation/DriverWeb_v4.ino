#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <DNSServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// Définition des broches moteurs
#define PWM_CHANNEL_1 0
#define PWM_CHANNEL_2 1
#define PWM_FREQ 1000
#define PWM_RESOLUTION 8
#define SV1_PIN 17  // Signal vitesse moteur 1. D10
#define SV2_PIN 25  // Signal vitesse moteur 2. D2
#define FR1_PIN 12  // Sens moteur 1. D13
#define FR2_PIN 26  // Sens moteur 2. D3
#define EN1_PIN 16  // Activation moteur 1. D11
#define EN2_PIN 13  // Activation moteur 2. D7
#define PG1_PIN 4   // Retour vitesse moteur 1. D12
#define PG2_PIN 14   // Retour vitesse moteur 2. D6
#define NUM_POLE_PAIRS 2

// Définir le nom et le mot de passe du point d'accès Wi-Fi
const char *ssid = "cesp32";
const char *password = "123456789";

// Définir l'adresse IP et le masque du réseau
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

// Serveur DNS
DNSServer dnsServer;

// Création du serveur web
WiFiServer server(80);

// Serveur WebSocket pour recevoir les données
WebSocketsServer webSocket = WebSocketsServer(81);

// Variables moteurs et de contrôle
volatile unsigned long pulseCount1 = 0, pulseCount2 = 0;
unsigned long lastRPMTime = 0;
float rpm1 = 0, rpm2 = 0;
const unsigned long rpmInterval = 100;
int speedY = 0, speedX = 0;  // Ces variables seront mises à jour via WebSocket

// Code HTML pour la page d'accueil
const char *homePage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Accueil BB-8</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body style="font-family: Arial, sans-serif; text-align: center; padding: 20px;">
    <h1>Bienvenue sur le controleur BB-8</h1>
    <p>Cliquez sur le bouton ci-dessous pour accéder au joystick.</p>
    <a href="/joystick" style="display: inline-block; padding: 10px 20px; background-color: #007BFF; color: white; text-decoration: none; border-radius: 5px; font-size: large;">Accéder au Joystick</a>
</body>
</html>
)rawliteral";

// Code HTML pour la page du joystick
const char *joystickPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>BB-8 Joystick</title>
    <meta name="viewport" content="user-scalable=no">
</head>
<body style="text-align: center; font-family: Arial, sans-serif;">
    <h1>BB-8 Controller</h1>
    <p>X: <span id="x_coordinate">0</span> Y: <span id="y_coordinate">0</span> Speed: <span id="speed">0</span>% Angle: <span id="angle">0</span></p>
    <canvas id="canvas" style="background-color: #eee; border: 1px solid #ccc;"></canvas>
    <script>
        const ws = new WebSocket('ws://' + window.location.hostname + ':81/');
        const canvas = document.getElementById('canvas');
        const ctx = canvas.getContext('2d');
        let width, height, radius, x_orig, y_orig;
    
        function resizeCanvas() {
            width = window.innerWidth * 0.9;
            height = window.innerHeight * 0.6;
            radius = 100;
            x_orig = width / 2;
            y_orig = height / 2;
            canvas.width = width;
            canvas.height = height;
            drawBackground();
            drawJoystick(x_orig, y_orig);
        }
    
        function drawBackground() {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            ctx.beginPath();
            ctx.arc(x_orig, y_orig, radius + 20, 0, Math.PI * 2);
            ctx.fillStyle = "#ECE5E5";
            ctx.fill();
        }
    
        function drawJoystick(x, y) {
            ctx.beginPath();
            ctx.arc(x, y, radius, 0, Math.PI * 2);
            ctx.fillStyle = "#F08080";
            ctx.fill();
        }
    
        let isDragging = false;
    
        function sendJoystickData(x, y) {
            const dx = Math.round(x - x_orig);
            const dy = Math.round(y - y_orig);
            const distance = Math.min(Math.sqrt(dx * dx + dy * dy), radius);
            const angle = Math.round((Math.atan2(dy, dx) * 180 / Math.PI + 360) % 360);
            const speed = Math.round((distance / radius) * 100);
    
            document.getElementById("x_coordinate").innerText = dx;
            document.getElementById("y_coordinate").innerText = dy;
            document.getElementById("speed").innerText = speed;
            document.getElementById("angle").innerText = angle;
    
            ws.send(JSON.stringify({ x: dx, y: dy, speed: speed, angle: angle }));
        }
    
        canvas.addEventListener('mousedown', (e) => {
            isDragging = true;
            handleJoystickMove(e);
        });
    
        canvas.addEventListener('mousemove', (e) => {
            if (isDragging) handleJoystickMove(e);
        });
    
        canvas.addEventListener('mouseup', () => {
            isDragging = false;
            drawBackground();
            drawJoystick(x_orig, y_orig);
            sendJoystickData(x_orig, y_orig);
        });
    
        function handleJoystickMove(e) {
            const rect = canvas.getBoundingClientRect();
            const x = Math.min(Math.max(e.clientX - rect.left, x_orig - radius), x_orig + radius);
            const y = Math.min(Math.max(e.clientY - rect.top, y_orig - radius), y_orig + radius);
            drawBackground();
            drawJoystick(x, y);
            sendJoystickData(x, y);
        }
    
        window.addEventListener('resize', resizeCanvas);
        resizeCanvas();
    </script>
</body>
</html>
)rawliteral";

// Prototypes des fonctions utilisées
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void updateRPM();
void updateMotors();

// Gestion des interruptions pour le comptage des pulses
void IRAM_ATTR onPulse1() { pulseCount1++; }
void IRAM_ATTR onPulse2() { pulseCount2++; }

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Configuration du point d'accès...");

    // Configurer le point d'accès Wi-Fi
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(ssid, password);
    Serial.print("Point d'accès démarré avec l'adresse IP : ");
    Serial.println(WiFi.softAPIP());

    // Configurer le serveur DNS
    dnsServer.start(53, "*", apIP);

    // Démarrer le serveur web et le WebSocket
    server.begin();
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    Serial.println("Serveur démarré.");

    // Configuration des moteurs
    ledcSetup(PWM_CHANNEL_1, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(PWM_CHANNEL_2, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(SV1_PIN, PWM_CHANNEL_1);
    ledcAttachPin(SV2_PIN, PWM_CHANNEL_2);
    pinMode(FR1_PIN, OUTPUT);
    pinMode(FR2_PIN, OUTPUT);
    pinMode(EN1_PIN, OUTPUT);
    pinMode(EN2_PIN, OUTPUT);
    pinMode(PG1_PIN, INPUT);
    pinMode(PG2_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(PG1_PIN), onPulse1, RISING);
    attachInterrupt(digitalPinToInterrupt(PG2_PIN), onPulse2, RISING);
}

void loop() {
    dnsServer.processNextRequest();
    webSocket.loop();
    updateRPM();
    updateMotors();

    // Gestion des requêtes HTTP
    WiFiClient client = server.available();
    if (client) {
        String request = "";
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                request += c;
                if (request.endsWith("\r\n\r\n")) { // Fin de l'en-tête HTTP
                    break;
                }
            }
        }
        if (request.indexOf("GET /joystick") >= 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type: text/html");
            client.println();
            client.print(joystickPage);
        } else {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type: text/html");
            client.println();
            client.print(homePage);
        }
        client.println();
        client.flush();
        client.stop();
    }
}

void updateRPM() {
    unsigned long currentTime = millis();
    if (currentTime - lastRPMTime >= rpmInterval) {
        float dt = (currentTime - lastRPMTime) / 1000.0;
        noInterrupts();
        unsigned long pulses1 = pulseCount1, pulses2 = pulseCount2;
        pulseCount1 = pulseCount2 = 0;
        interrupts();
        rpm1 = (pulses1 / dt * 60.0) / (3.0 * NUM_POLE_PAIRS);
        rpm2 = (pulses2 / dt * 60.0) / (3.0 * NUM_POLE_PAIRS);
        // Optionnel : affichage du RPM pour debug
        // Serial.printf("M1: %.2f RPM | M2: %.2f RPM\n", rpm1, rpm2);
        lastRPMTime = currentTime;
        Serial.printf("M1: %.2f RPM | M2: %.2f RPM\n", rpm1, rpm2);
    }
}

void updateMotors() {
    // Mise à jour de la direction et de la vitesse des moteurs
    // On suppose ici que speedY contrôle le moteur 1 et speedX le moteur 2.
    digitalWrite(FR1_PIN, speedY < 0 ? LOW : HIGH);
    digitalWrite(FR2_PIN, speedX < 0 ? LOW : HIGH);
    digitalWrite(EN1_PIN, speedY != 0 ? LOW : HIGH);
    digitalWrite(EN2_PIN, speedX != 0 ? LOW : HIGH);
    ledcWrite(PWM_CHANNEL_1, constrain(abs(speedY), 0, 255));
    ledcWrite(PWM_CHANNEL_2, constrain(abs(speedX), 0, 255));
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_TEXT) {
        String data = (char*)payload;

        // Parsing JSON des données reçues
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, data);
        if (!error) {
            int x = doc["x"];
            int y = doc["y"];
            int speed = doc["speed"];
            int angle = doc["angle"];
            Serial.printf("Données reçues -> x: %d, y: %d, speed: %d, angle: %d\n", x, y, speed, angle);
            Serial.printf("Vitesse moteur -> M1 PWM: %d | M2 PWM: %d\n", abs(speedY), abs(speedX));
            

            // Mise à jour des variables de commande moteur
            // Ici, on affecte 'x' au moteur 2 et 'y' au moteur 1
            // Mise à l'échelle de la vitesse (0-100 -> 0-255)
            speedX = map(x, -100, 100, -255, 255);
            speedY = map(y, -100, 100, -255, 255);
        } else {
            Serial.println("Erreur de parsing JSON");
        }
    }
}

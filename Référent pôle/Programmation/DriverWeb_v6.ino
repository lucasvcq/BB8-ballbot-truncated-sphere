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
#define SV1_PIN 26  // Signal vitesse moteur 1. D3
#define SV2_PIN 25  // Signal vitesse moteur 2. D2
#define FR1_PIN 4  // Sens moteur 1. D12
#define FR2_PIN 14  // Sens moteur 2. D6
#define EN1_PIN 16  // Activation moteur 1. D11
#define EN2_PIN 13  // Activation moteur 2. D7
#define PG1_PIN 12   // Retour vitesse moteur 1. D13
#define PG2_PIN 17   // Retour vitesse moteur 2. D10
#define NUM_POLE_PAIRS 2

// Nom et mot de passe du Wi-Fi
const char *ssid = "ESP32_BB8";
const char *password = "StarWars"; // Au moins 8 caractères obligatoires !

// Adresse IP du point d'accès
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

// Serveur DNS
DNSServer dnsServer;

// Serveur HTTP et WebSocket
WiFiServer server(80);
WebSocketsServer webSocket(81);

// Variables moteurs et de contrôle
volatile unsigned long pulseCount1 = 0, pulseCount2 = 0;
unsigned long lastRPMTime = 0;
float rpm1 = 0, rpm2 = 0;
const unsigned long rpmInterval = 100;
int speedY = 0, speedX = 0;  // Ces variables seront mises à jour via WebSocket

// Page HTML simplifiée pour tester la connexion
const char *Accueil = R"rawliteral(
<!DOCTYPE html>
<html>
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Accueil BB-8</title>
    </head>

    <body style="font-family: Arial;
    text-align: center; 
    padding: 20px;
    width: 70%;
    height: 25%;

    position: absolute;
    top: 25%; 
    left: 8%;

    border: 2px solid #ccc;
    border-radius: 10px;
    box-shadow: 4px 4px 10px rgba(0, 0, 0, 0.2);
    background-color: white;
    font-size: 16px;">
        <h1>Bienvenue sur le controleur BB8</h1>
        <a href="/Joystick" style="display: inline-block; 
        padding: 10px 20px; 
        background-color: #ff9900; 
        color: white; 
        text-decoration: none; 
        border-radius: 5px; 
        font-size: large;">Accéder au Joystick</a>
    </body>
</html>
)rawliteral";

const char *Joystick = R"rawliteral(
<!DOCTYPE html>
<html>
    <head>        
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Joystick BB-8</title>
    </head>

    <body style="display: flex;
    justify-content: center;
    align-items: center;
    height: 100vh;
    background-color: #f4f4f4;
    flex-direction: column;
    gap:0;

    font-family: Arial;
    text-align: center; 
    padding: 20px;
    width: 70%;
    height: 45%;

    position: absolute;
    top: 20%; 
    left: 8%;

    border: 2px solid #ccc;
    border-radius: 10px;
    box-shadow: 4px 4px 10px rgba(0, 0, 0, 0.2);
    background-color: white;
    font-size: 16px;
    resize: none;">
        <h2>BB8 Joystick</h2>
        <p style ="font-size: 18px">Position: (<span id="x_coordinate">0</span>,<span id="y_coordinate">0</span>)<br>
        Speed: <span id="speed">0</span>% | Angle: <span id="angle">0</span>°</p>
        <canvas id="canvas"></canvas>
                
        <script>
            const ws = new WebSocket('ws://' + location.hostname + ':81/');
            const canvas = document.getElementById('canvas');
            const ctx = canvas.getContext('2d');
            let width, height, radius, x_orig, y_orig;

            function resizeCanvas() {
                width = window.innerWidth * 0.45;
                height = window.innerHeight * 0.25;
                radius = 40;
                x_orig = width / 2;
                y_orig = height / 2;
                canvas.width = width;
                canvas.height = height;
                drawBackground();
                drawJoystick(x_orig, y_orig);
            }

            function drawBackground() {
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                ctx.fillStyle = "#FFFFFF"; // Fond blanc
                ctx.fillRect(0, 0, canvas.width, canvas.height);

                ctx.beginPath();
                ctx.arc(x_orig, y_orig, radius + 25, 0, Math.PI * 2);
                ctx.fillStyle = "#FFD580"; 
                ctx.fill();
            }

            function drawJoystick(x, y) {
                ctx.beginPath();
                ctx.arc(x, y, radius, 0, Math.PI * 2);
                ctx.fillStyle = "#FFA500";
                ctx.fill();
            }

            let isDragging = false;

            function sendJoystickData(x, y) {
                const raw_dx = x - x_orig;
                const raw_dy = y - y_orig;
                const maxDistance = radius + 15;
                const dx = Math.round((raw_dx / maxDistance) * 100);
                const dy = -Math.round((raw_dy / maxDistance) * 100);
                const distance = Math.min(Math.sqrt(dx * dx + dy * dy), 100);
                const angle = Math.round((Math.atan2(-raw_dy, raw_dx) * 180 / Math.PI + 360) % 360);
                const speed = Math.round(distance);


                document.getElementById("x_coordinate").innerText = dx;
                document.getElementById("y_coordinate").innerText = dy;
                document.getElementById("speed").innerText = speed;
                document.getElementById("angle").innerText = angle;

                if (ws.readyState === WebSocket.OPEN) {
                    ws.send(JSON.stringify({ x: dx, y: dy, speed: speed, angle: angle }));
                }
            }

            function handleJoystickMove(event) {
                const rect = canvas.getBoundingClientRect();
                const x = Math.min(Math.max(event.clientX - rect.left, x_orig - radius), x_orig + radius);
                const y = Math.min(Math.max(event.clientY - rect.top, y_orig - radius), y_orig + radius);
                drawBackground();
                drawJoystick(x, y);
                sendJoystickData(x, y);
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

            canvas.addEventListener('touchstart', (e) => {
                e.preventDefault();
                isDragging = true;
                handleJoystickMove(e.touches[0]);
            });

            canvas.addEventListener('touchmove', (e) => {
                e.preventDefault();
                if (isDragging) handleJoystickMove(e.touches[0]);
            });

            canvas.addEventListener('touchend', () => {
                isDragging = false;
                drawBackground();
                drawJoystick(x_orig, y_orig);
                sendJoystickData(x_orig, y_orig);
            });

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
    Serial.println("\n Démarrage de l'ESP32...");

    // Forcer l'ESP32 en mode Point d'accès (AP)
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, netMsk);

    // Démarrer le point d'accès Wi-Fi
    if (WiFi.softAP(ssid, password)) {
        Serial.println("Point d'accès activé !");
        Serial.print("SSID : ");
        Serial.println(ssid);
        Serial.print("Adresse IP : ");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.println("Échec de la création du point d'accès !");
        return;
    }

    // Démarrer le serveur DNS
    dnsServer.start(53, "*", apIP);

    // Démarrer le serveur Web et WebSocket
    server.begin();
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    Serial.println("Serveur Web et WebSocket démarrés.");

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

    WiFiClient client = server.available();
    if (client) {
        String request = "";
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                request += c;
                if (c == '\n' && request.endsWith("\r\n\r\n")) {
                break;
                }
                if (c == '\n') {
                    if (request.indexOf("GET /generate_204") >= 0 || 
                        request.indexOf("GET /hotspot-detect.html") >= 0 ||
                        request.indexOf("GET /ncsi.txt") >= 0) {

                        client.println("HTTP/1.1 302 Found");
                        client.println("Location: http://192.168.4.1/");
                        client.println("Connection: close");
                        client.println();
                        client.stop();
                        return;
                    }

                    if (request.indexOf("GET /Joystick") >= 0) {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();
                        client.print(Joystick);
                    } else {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();
                        client.print(Accueil);
                    }
                    client.println();
                    break;
                }
            }
        }
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

// Gestion des messages WebSocket
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
            speedX = map(x, -100, 100, -51, 51);
            speedY = map(y, -100, 100, -255, 255);
        } else {
            Serial.println("Erreur de parsing JSON");
        }
    }
}


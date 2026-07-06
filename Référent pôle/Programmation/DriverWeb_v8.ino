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
int currentSpeedY = 0;      // Vitesse actuelle moteur 1
const int rampStep = 5;     // Incrément de rampe (vitesse de transition)
unsigned long previousMillis = 0;
const int rampInterval = 20; // temps entre chaque incrément (en ms)

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
<html lang="fr">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Contrôle BB-8</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        text-align: center;
        background-color: #1e1e1e;
        color: #fff;
        margin: 0;
        padding: 20px;
      }
      h1 {
        margin-bottom: 30px;
      }
      .controls {
        display: flex;
        justify-content: center;
        align-items: center;
        gap: 40px;
      }
      .joystick {
        display: flex;
        flex-direction: column;
        align-items: center;
        gap: 15px;
      }
      input[type=range] {
        writing-mode: bt-lr;
        appearance: slider-vertical;
        width: 60px;     /* élargi */
        height: 300px;   /* plus haut */
        touch-action: none; /* meilleure précision sur mobile */
      }
      .buttons {
        display: flex;
        flex-direction: column;
        gap: 20px;
      }
      button {
        padding: 15px 25px;
        font-size: 18px;
        background-color: #2ecc71;
        color: white;
        border: none;
        border-radius: 10px;
        cursor: pointer;
        transition: background-color 0.2s;
        user-select: none;
        -webkit-user-select: none;
        -webkit-touch-callout: none;
        touch-action: manipulation;
      }
      button:active {
        background-color: #27ae60;
      }
      svg {
        width: 100px;
        height: 100px;
        margin-top: 30px;
      }
      html, body {
        overflow: hidden;
        height: 100%;
        margin: 0;
        padding: 0;
      }
    </style>
  </head>
<body>
  <h1>Contrôle BB-8</h1>
  <div class="controls">
    <div class="buttons">
      <button
        id="leftButton"
        draggable="false"
        ontouchstart="startMove(-100, 0)" 
        ontouchend="stopMove()"
        onmousedown="startMove(-100, 0)" 
        onmouseup="stopMove()"
      >Gauche</button>

      <button
        id="rightButton"
        draggable="false"
        ontouchstart="startMove(100, 0)" 
        ontouchend="stopMove()"
        onmousedown="startMove(100, 0)" 
        onmouseup="stopMove()"
      >Droite</button>

    </div>
    <div class="joystick">
      <input id="ySlider" type="range" min="-100" max="100" value="0" oninput="updateY(this.value)" />
      <label>Avant / Arrière</label>
    </div>
  </div>

  <svg viewBox="0 0 100 100">
    <polygon id="arrow" points="50,10 60,30 50,25 40,30" fill="rgb(46, 204, 113)" />
  </svg>

  <script>
    let socket;
    let xValue = 0;
    let yValue = 0;
    let sendInterval;

    window.onload = () => {
      socket = new WebSocket("ws://" + location.hostname + ":81");
      socket.onopen = () => {
        console.log("WebSocket connecté !");
        sendInterval = setInterval(sendJoystickData, 100); // ENVOI RÉGULIER
      };
      socket.onerror = e => console.error("Erreur WebSocket:", e);

      // Empêche scroll lors du toucher des boutons
      const leftButton = document.getElementById("leftButton");
      const rightButton = document.getElementById("rightButton");
      [leftButton, rightButton].forEach(button => {
        button.addEventListener("touchstart", e => e.preventDefault(), { passive: false });
      });
      // Reset du joystick à 0 quand on relâche
      const ySlider = document.getElementById("ySlider");
      const resetJoystick = () => {
        ySlider.value = 0;
        updateY(0);
      };
      ySlider.addEventListener("mouseup", resetJoystick);
      ySlider.addEventListener("touchend", resetJoystick);
    };

    function updateY(val) {
      yValue = parseInt(val);
      sendJoystickData();
    }

    function startMove(x, y) {
      xValue = x;
      yValue = yValue; // garde la valeur actuelle
      sendJoystickData();
      if (sendInterval) clearInterval(sendInterval);{
        sendInterval = setInterval(sendJoystickData, 100);
      }
    }

    function stopMove() {
      xValue = 0;
      sendJoystickData();
      clearInterval(sendInterval);
    }

    function getSpeedColor(x, y) {
      const speed = Math.sqrt(x * x + y * y); // 0 à ~141
      const normalized = Math.min(speed / 141, 1);
      const r = Math.round(46 + normalized * (231 - 46));   // 46 → 231
      const g = Math.round(204 - normalized * (204 - 76));  // 204 → 76
      const b = Math.round(113 - normalized * (113 - 60));  // 113 → 60
      return `rgb(${r}, ${g}, ${b})`;
    }

    function sendJoystickData() {
      if (!socket || socket.readyState !== WebSocket.OPEN) return;
      const data = { x: xValue, y: yValue };
      socket.send(JSON.stringify(data));

      // Mise à jour flèche
      const angle = Math.atan2(-yValue, xValue) * (180 / Math.PI);
      document.getElementById("arrow").style.transform = `rotate(${angle}deg)`;

      // Mise à jour couleur
      const color = getSpeedColor(xValue, yValue);
      document.getElementById("arrow").setAttribute("fill", color);
    }
  </script>
</body>
</html>
)rawliteral";


// Prototypes des fonctions utilisées
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void updateRPM();
void updateMotors();
void updateRampSpeedY();

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
    updateRampSpeedY();  // Rampe non bloquante
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

void updateRPM() {  //Calcule la vitesse réelle des moteurs (en RPM) à partir des pulses.
    unsigned long currentTime = millis();  // Récupère le temps actuel en millisecondes
    if (currentTime - lastRPMTime >= rpmInterval) {  // Vérifie si l'intervalle de temps est écoulé depuis la dernière mise à jour
        float dt = (currentTime - lastRPMTime) / 1000.0;  // Calcule le temps écoulé (en secondes) depuis la dernière mesure

        // Désactive les interruptions pour éviter des modifications concurrentes du compteur de pulses
        noInterrupts();
        unsigned long pulses1 = pulseCount1;  // Copie le nombre de pulses du moteur 1
        pulseCount1 = 0;                      // Réinitialise le compteur
        interrupts();                         // Réactive les interruptions

        noInterrupts();
        unsigned long pulses2 = pulseCount2;  // Idem pour le moteur 2
        pulseCount2 = 0;
        interrupts();

        // Calcule la vitesse de rotation (RPM) pour chaque moteur
        // Formule : (nombre de pulses / durée en secondes) * 60 secondes / (3 * nombre de paires de pôles)
        rpm1 = (pulses1 / dt * 60.0) / (3.0 * NUM_POLE_PAIRS);
        rpm2 = (pulses2 / dt * 60.0) / (3.0 * NUM_POLE_PAIRS);

        // Affiche les valeurs de RPM dans la console série pour le débogage
        Serial.printf("M1: %.2f RPM | M2: %.2f RPM\n", rpm1, rpm2);

        lastRPMTime = currentTime;  // Met à jour le temps de dernière mesure
    }
}


void updateMotors() {  //Applique les consignes de vitesse et de direction aux moteurs.
    // Contrôle du moteur 1 (avance/recul) avec vitesse progressive via currentSpeedY
    // Détermine le sens de rotation (avant ou arrière)
    digitalWrite(FR1_PIN, currentSpeedY >= 0 ? HIGH : LOW);
    // Active ou désactive le moteur (LOW pour activer selon le câblage)
    digitalWrite(EN1_PIN, currentSpeedY != 0 ? LOW : HIGH);
    // Envoie un signal PWM pour ajuster la vitesse
    ledcWrite(PWM_CHANNEL_1, constrain(abs(currentSpeedY), 0, 255));

    // Contrôle du moteur 2 (gauche/droite), sans rampe, directement selon speedX
    digitalWrite(FR2_PIN, speedX >= 0 ? HIGH : LOW);
    digitalWrite(EN2_PIN, speedX != 0 ? LOW : HIGH);
    ledcWrite(PWM_CHANNEL_2, constrain(abs(speedX), 0, 255));
}


void updateRampSpeedY() {  //Permet une montée ou descente progressive de la vitesse sur l'axe Y pour éviter les à-coups.
    unsigned long currentMillis = millis();  // Récupère le temps actuel
    if (currentMillis - previousMillis >= rampInterval) {  // Vérifie si le temps de lissage est écoulé
        previousMillis = currentMillis;  // Met à jour le temps de la dernière rampe

        // Augmente progressivement la vitesse actuelle vers la consigne (accélération douce)
        if (currentSpeedY < speedY) {
            currentSpeedY += rampStep;
            if (currentSpeedY > speedY) currentSpeedY = speedY;  // Ne pas dépasser la cible
        }
        // Diminue progressivement la vitesse actuelle vers la consigne (décélération douce)
        else if (currentSpeedY > speedY) {
            currentSpeedY -= rampStep;
            if (currentSpeedY < speedY) currentSpeedY = speedY;  // Ne pas descendre en dessous de la cible
        }
    }
}


// Fonction appelée automatiquement lorsqu'un événement WebSocket survient
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_TEXT) {  // Si le message reçu est un texte (JSON dans notre cas)
        
        // Création d’un document JSON statique pour parser le message
        StaticJsonDocument<128> doc;

        // Tentative de désérialisation du message JSON reçu
        DeserializationError error = deserializeJson(doc, payload, length);

        if (!error) {  // Si le parsing JSON a réussi

            // Lecture des valeurs "x" et "y" dans le message JSON
            int x = doc["x"];  // Valeur de commande horizontale (gauche/droite)
            int y = doc["y"];  // Valeur de commande verticale (avant/arrière)

            // Conversion (mapping) des valeurs -100 à 100 vers des valeurs PWM utilisables
            // Moteur 2 : rotation gauche/droite
            speedX = map(x, -100, 100, -255, 255);  

            // Moteur 1 : déplacement avant/arrière (plage réduite volontairement)
            speedY = map(y, -100, 100, -51, 51);  // Limitation de la vitesse max pour éviter d’aller trop vite

            // Affichage des valeurs reçues et des commandes PWM pour débogage
            Serial.printf("Reçu -> x: %d, y: %d\n", x, y);
            Serial.printf("PWM -> M1: %d | M2: %d\n", abs(speedY), abs(speedX));
        } else {
            // Message d'erreur si le parsing JSON échoue
            Serial.println("Erreur de parsing JSON");
         }
    }   
    else {
        // Gestion d'autres types de messages WebSocket (ex: binaires, ping/pong, etc.)
        Serial.printf("WebSocket non géré: %d\n", type);
    }
}

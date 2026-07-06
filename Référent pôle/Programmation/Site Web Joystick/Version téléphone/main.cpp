#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <DNSServer.h>
#include <WebSocketsServer.h>

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
    font-size: 16px;
    resize: none;">
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
                ctx.arc(x_orig, y_orig, radius + 15, 0, Math.PI * 2);
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
                const dx = Math.round((raw_dx / radius) * 100);
                const dy = -Math.round((raw_dy / radius) * 100);
                const distance = Math.min(Math.sqrt(dx * dx + dy * dy), radius);
                const angle = Math.round((Math.atan2(dy, dx) * 180 / Math.PI + 360) % 360);
                const speed = Math.round((distance / radius) * 100);

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

// Gestion des messages WebSocket
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_TEXT) {
        Serial.printf("Donnée reçue via WebSocket : %s\n", payload);
    }
}

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
}

void loop() {
    dnsServer.processNextRequest();
    webSocket.loop();

    WiFiClient client = server.available();
    if (client) {
        String request = "";
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                request += c;
                if (c == '\n' && request.length() == 0) {
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
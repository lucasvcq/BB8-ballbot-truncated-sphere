#include <Wire.h>
#include <SparkFun_BNO08x_Arduino_Library.h>

BNO08x myIMU;

void setup() {
    Serial.begin(115200);
    Wire.begin();

    if (!myIMU.begin()) {
        Serial.println("Erreur : BNO08x non détecté !");
        while (1);
    }

    Serial.println("BNO08x détecté !");
    
    // Activation du capteur d'accélération linéaire
    myIMU.enableLinearAccelerometer(50);  // Fréquence d'échantillonnage de 50Hz
}

void loop() {
    if (myIMU.wasReset()) {
        Serial.println("Le BNO08x a été réinitialisé !");
        myIMU.enableLinearAccelerometer(50);
    }

    if (myIMU.getSensorEvent()) {  // Vérifie si de nouvelles données sont disponibles
        float x = myIMU.getLinAccelX();
        float y = myIMU.getLinAccelY();
        float z = myIMU.getLinAccelZ();

        Serial.print("Accélération (m/s²) : X=");
        Serial.print(x, 2);
        Serial.print(" Y=");
        Serial.print(y, 2);
        Serial.print(" Z=");
        Serial.println(z, 2);
    }

    delay(100);
}

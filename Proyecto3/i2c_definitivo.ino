/*
  PROYECTO 3 - DIGITAL 2
  Parqueo inteligente con acceso Web
  Adrián Fernández 23381
  Brittany Herdez 17386
*/

#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>

#define I2CSlaveAddress1 0x18
#define I2CSlaveAddress2 0x19

#define I2C_SDA 21
#define I2C_SCL 22

/* Put your SSID & Password */
const char* ssid = "parqueo";  // Enter SSID here
const char* password = "parqueoo";  //Enter Password here

/* Put IP Address details */
IPAddress local_ip(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);

WebServer server(80);

uint8_t error = 0;
uint8_t parqueos[8] = {0};

unsigned long ultimoI2C = 0;

// External Wire.h equivalent error Codes
typedef enum {
  I2C_ERROR_OK=0,
  I2C_ERROR_DEV,
  I2C_ERROR_ACK,
  I2C_ERROR_TIMEOUT,
  I2C_ERROR_BUS,
  I2C_ERROR_BUSY,
  I2C_ERROR_MEMORY,
  I2C_ERROR_CONTINUE,
  I2C_ERROR_NO_BEGIN
} i2c_err_t;

void setup()
{
  // Se inicia el I2C
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  Serial.println("\nInicializando Sistema I2C ESP32 (Master)");

  // Se inicia el web server
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  
  // Transimitir datos al web server
  server.on("/", handle_OnConnect);
  server.on("/estado", handle_Estado);
  server.onNotFound(handle_NotFound);
  
  server.begin();
  Serial.println("HTTP server started");
}



void loop() {
  server.handleClient();
 
  if (millis() - ultimoI2C >= 500) {
    ultimoI2C = millis();
//------------------ Primer I2C ------------------------

    Wire.beginTransmission(I2CSlaveAddress1);
    
    Wire.write(parqueos[0]);    // Escribe los datos del parqueo 
    Wire.write(parqueos[1]);    // Escribe los datos del parqueo
    Wire.write(parqueos[2]);    // Escribe los datos del parqueo
    Wire.write(parqueos[3]);    // Escribe los datos del parqueo

    error = Wire.endTransmission(true); // true para enviar STOP
    Serial.print(" endTransmission: ");
    Serial.println(error);

    // Leer 4 bytes del esclavo
    uint8_t bytesReceived = Wire.requestFrom(I2CSlaveAddress1, 4);

    Serial.print("requestFrom: ");
    Serial.println(bytesReceived);
    if (bytesReceived == 4) {  
        // Si recibimos exactamente 4, los guardamos en la segunda mitad del arreglo
        parqueos[4] = Wire.read();    // Lee los datos del parqueo
        parqueos[5] = Wire.read();    // Lee los datos del parqueo
        parqueos[6] = Wire.read();    // Lee los datos del parqueo
        parqueos[7] = Wire.read();    // Lee los datos del parqueo
      } else {
        Serial.println("Advertencia: No se recibieron los 4 bytes esperados");
        // Limpiamos el buffer por si acaso (para no dejar basura en el bus)
        while(Wire.available()){
          Wire.read(); 
        }
      }

    Wire.endTransmission(I2CSlaveAddress1);

  //------------------ Segundo I2C ------------------------
    Wire.beginTransmission(I2CSlaveAddress2);
    
    Wire.write(parqueos[4]);    // Escribe los datos del parqueo
    Wire.write(parqueos[5]);    // Escribe los datos del parqueo
    Wire.write(parqueos[6]);    // Escribe los datos del parqueo
    Wire.write(parqueos[7]);    // Escribe los datos del parqueo

    error = Wire.endTransmission(true); // true para enviar STOP
    Serial.print(" endTransmission: ");
    Serial.println(error);

    // Leer 4 bytes del esclavo
    uint8_t bytesReceived_2 = Wire.requestFrom(I2CSlaveAddress2, 4);

    Serial.print("requestFrom: ");
    Serial.println(bytesReceived_2);
    if (bytesReceived_2 == 4) {  
        // Si recibimos exactamente 4, los guardamos en la segunda mitad del arreglo
        parqueos[0] = Wire.read();    // Lee los datos del parqueo
        parqueos[1] = Wire.read();    // Lee los datos del parqueo
        parqueos[2] = Wire.read();    // Lee los datos del parqueo
        parqueos[3] = Wire.read();    // Lee los datos del parqueo
      } else {
        Serial.println("Advertencia: No se recibieron los 4 bytes esperados");
        // Limpiamos el buffer por si acaso (para no dejar basura en el bus)
        while(Wire.available()){
          Wire.read(); 
        }
      }

    Wire.endTransmission(I2CSlaveAddress2);

  // --------------- Fin de comunicación de esclavos ----------

    Serial.print("Estado Global del Parqueo: [");
    for (int i = 0; i < 8; i++) {
      Serial.print(parqueos[i]);
      if (i < 7) Serial.print(", ");
    }
    Serial.println("]\n");
  }
}

// Se comparte el estado del buffer de parqueos con el web server
void handle_Estado() {
  // Se envia bit por bit cada uno de los estados de los parqueos.
  String json = "{\"p\":[";
  for (int i = 0; i < 8; i++) {
    json += String(parqueos[i] ? 1 : 0);    // Se envia un 1 o 0 a la pagina
    if (i < 7) json += ",";
  }
  json += "]}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);   // se evian los datos a la pagina
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}
void handle_OnConnect() {
  server.send(200, "text/plain", "ESP32 activo");
}
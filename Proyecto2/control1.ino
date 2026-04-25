/*
 * control1.ino  —  ESP32 Hub de controles (transmisor ESP-NOW)
 *
 * ===========================================================================
 * Qué hace este archivo:
 *   Recibe comandos de los dos controles ATmega por UART y los reenvía
 *   de forma inalámbrica al ESP32 de la consola usando ESP-NOW.
 *
 * Conexiones UART:
 *   Control 1 (ATmega) → Serial2 (RX=16, TX=17)
 *   Control 2 (ATmega) → Serial1 (RX=25, TX=26)
 *
 * Protocolo de envío:
 *   Cada paquete tiene 2 bytes: número de jugador ('1' o '2') + comando (U/D/L/R/A/B).
 *   Solo se envía cuando el comando CAMBIA. Si el joystick sigue igual, no se manda nada.
 *
 * Timeout:
 *   Si no llega nada de un control por más de 100 ms, se asume que está en reposo ('C').
 *   Esto evita que queden comandos "pegados" si se pierde un byte por UART.
 * ===========================================================================
 */

#include <esp_now.h>
#include <WiFi.h>

/* ===========================================================================
 * SECCIÓN: PINES UART
 * ===========================================================================*/
#define RXD2 16  /* RX para el Control 1 (ATmega 1) */
#define TXD2 17  /* TX para el Control 1 */

#define RXD1 25  /* RX para el Control 2 (ATmega 2) */
#define TXD1 26  /* TX para el Control 2 */

/* ===========================================================================
 * SECCIÓN: VARIABLES DE ESTADO
 * ===========================================================================*/

/* Último comando recibido de cada control ('C' = ninguno / reposo) */
char cmd1 = 'C';
char cmd2 = 'C';

/* Último comando que se envió por ESP-NOW (para detectar cambios) */
char prev1 = 'X'; /* 'X' para forzar un primer envío al arrancar */
char prev2 = 'X';

/* Tiempo del último byte recibido por UART (para el timeout de 100 ms) */
unsigned long t1 = 0;
unsigned long t2 = 0;

/* ===========================================================================
 * SECCIÓN: ESTRUCTURA DE DATOS ESP-NOW
 * Cada paquete tiene solo 2 bytes: jugador + comando.
 * ===========================================================================*/
typedef struct {
  char player;   /* '1' o '2' */
  char command;  /* 'U', 'D', 'L', 'R', 'A' o 'B' */
} Data;

Data data; /* Paquete que se envía por ESP-NOW */

/* ===========================================================================
 * SECCIÓN: CONFIGURACIÓN ESP-NOW
 * MAC del ESP32 receptor (el que está en la consola).
 * Hay que actualizarla si se cambia el ESP32 receptor.
 * ===========================================================================*/
uint8_t receiverMAC[] = {0xC0, 0xCD, 0xD6, 0x8E, 0x38, 0xAC};

/* ---------------------------------------------------------------------------
 * sendData()
 * Arma el paquete y lo manda por ESP-NOW al receptor.
 *
 * Parámetros:
 *   player — quién manda el comando: '1' o '2'
 *   cmd    — el comando: 'U', 'D', 'L', 'R', 'A' o 'B'
 * ---------------------------------------------------------------------------*/
void sendData(char player, char cmd) {
  data.player  = player;
  data.command = cmd;
  esp_now_send(receiverMAC, (uint8_t *) &data, sizeof(data));
}

/* ===========================================================================
 * SECCIÓN: SETUP — Se ejecuta una vez al arrancar
 * ===========================================================================*/
void setup() {
  Serial.begin(115200); /* Puerto serial para debug */

  /* Abrir los dos puertos UART para leer los controles */
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2); /* Control 1 */
  Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1); /* Control 2 */

  /* ESP-NOW necesita que el WiFi esté en modo estación (STA) */
  WiFi.mode(WIFI_STA);

  /* Inicializar ESP-NOW */
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error ESP-NOW");
    return;
  }

  /* Registrar al receptor como peer (destinatario conocido) */
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false; /* Sin cifrado para simplificar */
  esp_now_add_peer(&peerInfo);
}

/* ===========================================================================
 * SECCIÓN: LOOP — Se ejecuta continuamente
 * Lee UART de los dos controles y reenvía cambios por ESP-NOW.
 * ===========================================================================*/
void loop() {

  /* ---- CONTROL 1 ----
   * Leer UART del ATmega 1. Si llega algo, guardarlo y resetear el timer.
   * Si pasan más de 100 ms sin recibir nada, asumir posición central ('C'). */
  if (Serial2.available()) {
    cmd1 = Serial2.read(); /* Leer byte del control 1 */
    t1 = millis();         /* Registrar cuándo llegó */
  }
  if (millis() - t1 > 100) cmd1 = 'C'; /* Timeout: volver a reposo */

  /* Solo mandar por ESP-NOW si el comando cambió y no es reposo */
  if (cmd1 != prev1 && cmd1 != 'C') {
    sendData('1', cmd1);
  }
  prev1 = cmd1; /* Guardar para la próxima iteración */

  /* ---- CONTROL 2 ----
   * Mismo proceso que el control 1, pero con Serial1. */
  if (Serial1.available()) {
    cmd2 = Serial1.read(); /* Leer byte del control 2 */
    t2 = millis();
  }
  if (millis() - t2 > 100) cmd2 = 'C'; /* Timeout: volver a reposo */

  /* Solo mandar por ESP-NOW si el comando cambió y no es reposo */
  if (cmd2 != prev2 && cmd2 != 'C') {
    sendData('2', cmd2);
  }
  prev2 = cmd2;

  delay(20); /* Pequeña pausa para no saturar el bus ESP-NOW */
}

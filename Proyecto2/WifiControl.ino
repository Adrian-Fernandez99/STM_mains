/*
 * WifiControl.ino  —  ESP32 Receptor ESP-NOW (lado de la consola)
 *
 * ===========================================================================
 * Qué hace este archivo:
 *   Recibe los paquetes ESP-NOW que manda el ESP32 hub de los controles
 *   y los reenvía al STM32 de la consola por UART.
 *
 * El paquete tiene 2 bytes: número de jugador ('1' o '2') + comando (U/D/L/R/A/B).
 *
 * Por ahora el loop está vacío porque toda la lógica es manejada por la
 * función de callback OnDataRecv, que se llama automáticamente cada vez
 * que llega un paquete por ESP-NOW.
 * ===========================================================================
 */

#include <esp_now.h>
#include <WiFi.h>

/* ===========================================================================
 * SECCIÓN: ESTRUCTURA DE DATOS ESP-NOW
 * Debe ser idéntica a la del archivo del transmisor (control1.ino).
 * Si no coinciden, los datos recibidos se van a interpretar mal.
 * ===========================================================================*/
typedef struct {
  char player;   /* '1' o '2' — qué jugador mandó el comando */
  char command;  /* 'U', 'D', 'L', 'R', 'A' o 'B' */
} Data;

Data data; /* Aquí se copian los datos del paquete recibido */

/* ===========================================================================
 * SECCIÓN: CALLBACK DE RECEPCIÓN
 * ===========================================================================*/

/* ---------------------------------------------------------------------------
 * OnDataRecv()
 * Esta función se llama automáticamente cada vez que llega un paquete ESP-NOW.
 * No hay que llamarla manualmente, el stack de ESP-NOW la dispara solo.
 *
 * Parámetros (los pone ESP-NOW automáticamente):
 *   info         — información del remitente (MAC, canal, etc.)
 *   incomingData — bytes del paquete recibido
 *   len          — cantidad de bytes recibidos
 * ---------------------------------------------------------------------------*/
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  /* Copiar los bytes recibidos a nuestra estructura */
  memcpy(&data, incomingData, sizeof(data));

  /* Mostrar el comando recibido por Serial para debug.
   * En el proyecto final esto se reemplazaría por un envío UART al STM32. */
  Serial.print("P");
  Serial.print(data.player);   /* Jugador: 1 o 2 */
  Serial.print(": ");

  /* Traducir el comando a texto legible */
  switch(data.command) {
    case 'U': Serial.println("Arriba");     break;
    case 'D': Serial.println("Abajo");      break;
    case 'L': Serial.println("Izquierda");  break;
    case 'R': Serial.println("Derecha");    break;
    case 'A': Serial.println("Boton A");    break;
    case 'B': Serial.println("Boton B");    break;
  }
}

/* ===========================================================================
 * SECCIÓN: SETUP — Se ejecuta una vez al arrancar
 * ===========================================================================*/
void setup() {
  Serial.begin(115200);
  delay(1000); /* Esperar a que el Serial esté listo antes de iniciar ESP-NOW */

  /* ESP-NOW necesita que el WiFi esté en modo estación */
  WiFi.mode(WIFI_STA);

  /* Inicializar ESP-NOW */
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error ESP-NOW");
    return;
  }

  /* Registrar la función que se llama cuando llega un paquete */
  esp_now_register_recv_cb(OnDataRecv);
}

/* ===========================================================================
 * SECCIÓN: LOOP — Vacío porque todo lo maneja el callback OnDataRecv
 * ===========================================================================*/
void loop() {
  /* No hay nada que hacer aquí. ESP-NOW llama a OnDataRecv automáticamente. */
}

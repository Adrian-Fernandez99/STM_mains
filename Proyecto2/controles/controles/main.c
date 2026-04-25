/*
 * main.c  —  Control físico (ATmega328P)
 * Autor: Britany Barillas
 * Creado: 3/26/2026
 *
 * ===========================================================================
 * Qué hace este archivo:
 *   Lee un joystick analógico (2 ejes) y dos botones físicos.
 *   Convierte esas lecturas en comandos de un solo carácter y los manda
 *   por UART al ESP32 hub a 115200 baudios.
 *
 * Comandos que manda:
 *   'U' — joystick arriba
 *   'D' — joystick abajo
 *   'L' — joystick izquierda
 *   'R' — joystick derecha
 *   'A' — botón A presionado
 *   'B' — botón B presionado
 *
 * Solo manda un comando cuando CAMBIA el estado.
 * Si el joystick sigue en la misma posición, no manda nada.
 * Los botones solo disparan al presionar (flanco de bajada), no mientras están held.
 *
 * Pines usados:
 *   ADC canal 0 (PC0) — eje X del joystick
 *   ADC canal 1 (PC1) — eje Y del joystick
 *   PD6               — botón A (con pull-up interno)
 *   PD7               — botón B (con pull-up interno)
 *   TX (PD1)          — UART hacia el ESP32
 * ===========================================================================
 */
 
#define F_CPU 16000000UL  /* Frecuencia del ATmega: 16 MHz */
 
#include <avr/io.h>
#include <util/delay.h>
 
/* ===========================================================================
 * SECCIÓN: UART
 * Comunicación serial hacia el ESP32 a 115200 baudios.
 * Se usa U2X (doble velocidad) para lograr 115200 con el oscilador de 16 MHz.
 * ===========================================================================*/
 
/* ---------------------------------------------------------------------------
 * UART_init()
 * Configura el UART0 del ATmega para transmitir a 115200 baudios.
 * UBRR = 16 con U2X activo da exactamente 115200 baud a 16 MHz.
 * Solo se activa el transmisor (TXEN0) porque no necesitamos recibir nada.
 * ---------------------------------------------------------------------------*/
void UART_init(void)
{
    uint16_t ubrr = 16; /* 115200 baud con U2X */
    UCSR0A = (1 << U2X0);           /* Activar modo doble velocidad */
    UBRR0H = (ubrr >> 8);           /* Parte alta del baud rate */
    UBRR0L = ubrr;                  /* Parte baja del baud rate */
    UCSR0B = (1 << TXEN0);          /* Habilitar solo el transmisor */
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); /* Formato: 8 bits, sin paridad, 1 stop */
}
 
/* ---------------------------------------------------------------------------
 * UART_sendChar()
 * Manda un solo carácter por UART.
 * Espera (bloqueante) a que el buffer de transmisión esté libre antes de escribir.
 *
 * Parámetro: data — carácter a mandar ('U', 'D', 'L', 'R', 'A' o 'B')
 * ---------------------------------------------------------------------------*/
void UART_sendChar(char data)
{
    while (!(UCSR0A & (1 << UDRE0))); /* Esperar a que el buffer esté libre */
    UDR0 = data;                       /* Escribir el carácter */
}
 
/* ===========================================================================
 * SECCIÓN: ADC (joystick analógico)
 * El joystick da voltajes entre 0 y 5V según su posición.
 * El ADC del ATmega los convierte a números de 0 a 1023.
 * ===========================================================================*/
 
/* ---------------------------------------------------------------------------
 * ADC_init()
 * Configura el ADC:
 *   - Referencia: AVcc (5V)
 *   - Prescaler: 128 ? frecuencia ADC = 16MHz / 128 = 125 kHz (dentro del rango recomendado)
 * ---------------------------------------------------------------------------*/
void ADC_init(void)
{
    ADMUX  = (1 << REFS0);  /* Referencia de voltaje: AVcc */
    ADCSRA = (1 << ADEN) |
             (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); /* Habilitar ADC, prescaler 128 */
}
 
/* ---------------------------------------------------------------------------
 * ADC_read()
 * Hace una conversión en el canal indicado y devuelve el resultado (0-1023).
 *
 * Parámetro: channel — canal ADC a leer (0 para eje X, 1 para eje Y)
 * Retorna: valor ADC de 10 bits (0 = 0V, 1023 = 5V)
 * ---------------------------------------------------------------------------*/
uint16_t ADC_read(uint8_t channel)
{
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F); /* Seleccionar canal */
    ADCSRA |= (1 << ADSC);                      /* Iniciar conversión */
    while (ADCSRA & (1 << ADSC));               /* Esperar a que termine */
    return ADC;                                  /* Devolver resultado */
}
 
/* ===========================================================================
 * SECCIÓN: BOTONES
 * Los botones se conectan entre el pin y GND.
 * El pull-up interno los mantiene en alto cuando están sueltos.
 * Cuando se presionan, el pin va a GND (LOW).
 * ===========================================================================*/
 
/* ---------------------------------------------------------------------------
 * Buttons_init()
 * Configura PD6 y PD7 como entradas con pull-up interno.
 * DDRx = 0 ? entrada. PORTx = 1 ? pull-up activado.
 * ---------------------------------------------------------------------------*/
void Buttons_init(void)
{
    DDRD  &= ~((1 << PD6) | (1 << PD7)); /* PD6 y PD7 como entrada */
    PORTD |=  (1 << PD6) | (1 << PD7);  /* Activar pull-ups internos */
}
 
/* ===========================================================================
 * SECCIÓN: MAIN — Loop principal de lectura y envío de comandos
 * ===========================================================================*/
int main(void)
{
    /* Inicializar todos los periféricos */
    UART_init();
    ADC_init();
    Buttons_init();
 
    /* Estados anteriores — para detectar cambios y no mandar comandos repetidos */
    static char lastX = 0;   /* Última dirección del eje X enviada */
    static char lastY = 0;   /* Última dirección del eje Y enviada */
    static uint8_t lastA = 1; /* Estado anterior del botón A (1 = suelto) */
    static uint8_t lastB = 1; /* Estado anterior del botón B (1 = suelto) */
 
    while (1)
    {
        /* Leer ambos ejes del joystick */
        uint16_t x = ADC_read(0); /* Eje X: canal ADC0 */
        uint16_t y = ADC_read(1); /* Eje Y: canal ADC1 */
 
        /* ---- JOYSTICK EJE X (detecta Arriba / Abajo) ----
         * Rango: 0-1023
         *   < 300  ? joystick empujado hacia abajo  ? 'D'
         *   > 700  ? joystick empujado hacia arriba ? 'U'
         *   300-700 ? posición central              ? sin comando */
        char currentX = 0;
        if (x < 300)       currentX = 'D';
        else if (x > 700)  currentX = 'U';
        else               currentX = 0; /* Centro */
 
        /* Solo mandar si cambió la dirección */
        if (currentX != lastX)
        {
            if (currentX != 0)
                UART_sendChar(currentX); /* Mandar 'U' o 'D' */
        }
        lastX = currentX;
 
        /* ---- JOYSTICK EJE Y (detecta Izquierda / Derecha) ----
         *   < 300  ? joystick hacia la izquierda ? 'L'
         *   > 700  ? joystick hacia la derecha   ? 'R'
         *   300-700 ? posición central            ? sin comando */
        char currentY = 0;
        if (y < 300)       currentY = 'L';
        else if (y > 700)  currentY = 'R';
        else               currentY = 0; /* Centro */
 
        /* Solo mandar si cambió la dirección */
        if (currentY != lastY)
        {
            if (currentY != 0)
                UART_sendChar(currentY); /* Mandar 'L' o 'R' */
        }
        lastY = currentY;
 
        /* ---- BOTONES (detección por flanco de bajada) ----
         * Se detecta solo el momento en que el botón se PRESIONA,
         * no mientras está held. Eso evita mandar comandos repetidos.
         * Flanco de bajada: el estado anterior era 1 (suelto) y ahora es 0 (presionado). */
        uint8_t currentA = PIND & (1 << PD6); /* Leer estado actual del botón A */
        uint8_t currentB = PIND & (1 << PD7); /* Leer estado actual del botón B */
 
        /* Botón A: mandar 'A' solo al presionar (flanco bajada) */
        if (!currentA && lastA)
        {
            UART_sendChar('A');
        }
 
        /* Botón B: mandar 'B' solo al presionar (flanco bajada) */
        if (!currentB && lastB)
        {
            UART_sendChar('B');
        }
 
        /* Guardar estados para la siguiente iteración */
        lastA = currentA;
        lastB = currentB;
 
        /* Esperar 100 ms antes del siguiente ciclo de lectura.
         * Esto da tiempo al usuario para mover el joystick y evita lecturas ruidosas. */
        _delay_ms(100);
    }
}
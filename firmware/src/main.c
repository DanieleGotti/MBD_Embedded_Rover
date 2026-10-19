/*
 * LED di stato (D13 = PB5) lampeggiante con il tick di sistema a 1 ms.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#include "mcal/tick.h"
#include "mcal/uart.h"


int main(void)
{
    DDRB |= (1 << PB5);      /* PB5 uscita */
    uint32_t last500 = 0;
    uint32_t last1000 = 0;
    char buf[11];   
 
    Tick_Init();
    Uart_Init();

    sei();                   /* abilita gli interrupt dopo aver inizializzato tutto */

    Uart_PutString("MBD_Embedded_Rover avviato\r\n");

    for (;;) {
        uint32_t now = Tick_GetMs();

        if ((uint32_t)(now - last500) >= 500) {
            last500 = now;
            PINB = (1 << PB5);   /* inversione atomica di PB5 */
        }

        if ((uint32_t)(now - last1000) >= 1000) {
            last1000 = now;

            uint32_t start = Tick_GetMs();
            ultoa(now, buf, 10);
            Uart_PutString("t = ");
            Uart_PutString(buf);
            Uart_PutString(" ms");
            uint32_t elapsed = Tick_GetMs() - start;

            ultoa(elapsed, buf, 10);
            Uart_PutString(" (stampa: ");
            Uart_PutString(buf);
            Uart_PutString(" ms)\r\n");
        }

        /* Eco: ogni carattere ricevuto viene rimandato indietro */
        char c;
        if (Uart_GetChar(&c)) {
            Uart_PutChar(c);
        }
    }
}

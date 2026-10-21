/*
 * uart — implementazione a interrupt con due buffer circolari.
 *
 * Ogni buffer ha un solo scrittore e un solo lettore:
 *   TX: scrive il codice principale (head), legge l'ISR UDRE (tail)
 *   RX: scrive l'ISR RX (head), legge il codice principale (tail)
 * Gli indici sono uint8_t: su AVR leggerli e scriverli è atomico.
 */
#include "uart.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#define UBRR_VALUE ((F_CPU / (16UL * UART_BAUD)) - 1UL)

#define TX_SIZE 256u                /* potenza di 2; con uint8_t al massimo 256 */
#define RX_SIZE 32u                 /* potenza di 2 */
#define TX_MASK (TX_SIZE - 1u)
#define RX_MASK (RX_SIZE - 1u)

static volatile char    s_tx_buf[TX_SIZE];
static volatile uint8_t s_tx_head = 0;   /* prossima posizione da scrivere */
static volatile uint8_t s_tx_tail = 0;   /* prossima posizione da leggere */

static volatile char    s_rx_buf[RX_SIZE];
static volatile uint8_t s_rx_head = 0;
static volatile uint8_t s_rx_tail = 0;

static volatile uint16_t s_tx_dropped = 0;

void Uart_Init(void)
{
    UBRR0  = UBRR_VALUE;
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
}

bool Uart_PutChar(char c)
{
    uint8_t s_tx_next = (s_tx_head + 1) & TX_MASK;
    if (s_tx_next == s_tx_tail) {
        s_tx_dropped++;
        return false;
    }
    s_tx_buf[s_tx_head] = c;
    s_tx_head = s_tx_next;
    UCSR0B |= (1 << UDRIE0);
    return true;
}

void Uart_PutString(const char *s)
{
    while (*s != '\0') {
        Uart_PutChar(*s);
        s++;
    }
}

bool Uart_GetChar(char *c)
{
    if (s_rx_head == s_rx_tail) {
        return false;
    }
    *c = s_rx_buf[s_rx_tail];
    s_rx_tail = (s_rx_tail + 1) & RX_MASK;
    return true;
}

uint16_t Uart_GetTxDropped(void)
{
    uint16_t dropped;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        dropped = s_tx_dropped;
    }
    return dropped;
}

ISR(USART_UDRE_vect)
{
    if (s_tx_head == s_tx_tail) {
        UCSR0B &= ~(1 << UDRIE0);
        return;
    }
    UDR0 = s_tx_buf[s_tx_tail];
    s_tx_tail = (s_tx_tail + 1) & TX_MASK;
}

ISR(USART_RX_vect)
{
    char data = UDR0;
    uint8_t next = (s_rx_head + 1) & RX_MASK;

    if (next != s_rx_tail) {
        s_rx_buf[s_rx_head] = data;
        s_rx_head = next;
    }
}

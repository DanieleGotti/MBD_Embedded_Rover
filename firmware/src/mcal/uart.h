/*
 * uart — USART0, 8N1. Versione 2: trasmissione e ricezione a interrupt
 * con buffer circolari. Nessuna funzione resta in attesa.
 *
 * Pin: D0 = RXD (PD0), D1 = TXD (PD1).
 * Risorse: interrupt USART_RX e USART_UDRE.
 */
#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

#define UART_BAUD 9600UL

/* Configura USART0 e abilita l'interrupt di ricezione. */
void Uart_Init(void);

/*
 * Mette un byte nel buffer di trasmissione e ritorna subito.
 * Ritorna false se il buffer è pieno: il byte viene scartato e contato.
 */
bool Uart_PutChar(char c);

/* Mette una stringa nel buffer di trasmissione (carattere per carattere). */
void Uart_PutString(const char *s);

/*
 * Se c'è un byte ricevuto lo scrive in *c e ritorna true,
 * altrimenti ritorna false senza attendere.
 */
bool Uart_GetChar(char *c);

/* Numero di byte scartati perché il buffer di trasmissione era pieno. */
uint16_t Uart_GetTxDropped(void);

#endif /* UART_H */

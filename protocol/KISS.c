#include <stdlib.h>
#include <string.h>

#include "device.h"
#include "hardware/Serial.h"
#include "hardware/LED.h"
#include "util/FIFO16.h"
#include "util/time.h"
#include "KISS.h"

uint8_t packet_queue[CONFIG_QUEUE_SIZE];
uint8_t tx_buffer[AX25_MAX_FRAME_LEN];
volatile uint8_t queue_height = 0;
volatile size_t queued_bytes = 0;
volatile size_t queue_cursor = 0;
volatile size_t current_packet_start = 0;

FIFOBuffer16 packet_starts;
size_t packet_starts_buf[CONFIG_QUEUE_MAX_LENGTH+1];

FIFOBuffer16 packet_lengths;
size_t packet_lengths_buf[CONFIG_QUEUE_MAX_LENGTH+1];

AX25Ctx *ax25ctx;
Afsk *channel;
Serial *serial;
volatile ticks_t last_serial_read = 0;
size_t frame_len;
bool IN_FRAME;
bool ESCAPE;

uint8_t command = CMD_UNKNOWN;
unsigned long custom_preamble = CONFIG_AFSK_PREAMBLE_LEN;
unsigned long custom_tail = CONFIG_AFSK_TRAILER_LEN;

unsigned long slotTime = 200;
uint8_t p = CONFIG_CSMA_P;

void kiss_init(AX25Ctx *ax25, Afsk *afsk, Serial *ser) {
    ax25ctx = ax25;
    serial = ser;
    channel = afsk;

    memset(packet_queue, 0, sizeof(packet_queue));
    memset(packet_starts_buf, 0, sizeof(packet_starts));
    memset(packet_lengths_buf, 0, sizeof(packet_lengths));

    fifo16_init(&packet_starts, packet_starts_buf, sizeof(packet_starts_buf));
    fifo16_init(&packet_lengths, packet_lengths_buf, sizeof(packet_lengths_buf));
}

void kiss_poll(void) {
    while (!fifo_isempty_locked(&serialFIFO)) {
        char sbyte = fifo_pop_locked(&serialFIFO);
        kiss_serialCallback(sbyte);
        last_serial_read = timer_clock();
    }
}

// TODO: Remove debug functions
//size_t decodes = 0;
void kiss_messageCallback(AX25Ctx *ctx) {
    //decodes++;
    //printf("%d\r\n", decodes);
    
    fputc(FEND, &serial->uart0);
    fputc(0x00, &serial->uart0);
    for (unsigned i = 0; i < ctx->frame_len-2; i++) {
        uint8_t b = ctx->buf[i];
        if (b == FEND) {
            fputc(FESC, &serial->uart0);
            fputc(TFEND, &serial->uart0);
        } else if (b == FESC) {
            fputc(FESC, &serial->uart0);
            fputc(TFESC, &serial->uart0);
        } else {
            fputc(b, &serial->uart0);
        }
    }
    fputc(FEND, &serial->uart0);
    
}

void kiss_csma(void) {
    if (queue_height > 0) {
        #if BITRATE == 2400
            if (!channel->hdlc.dcd) {
                ticks_t timeout = last_serial_read + ms_to_ticks(CONFIG_SERIAL_TIMEOUT_MS);
                if (timer_clock() > timeout) {
                    if (p == 255) {
                        kiss_flushQueue();
                    } else {
                        // TODO: Implement real CSMA
                    }
                }
            }
        #else
            if (!channel->hdlc.dcd) {
                if (p == 255) {
                    kiss_flushQueue();
                } else {
                    // TODO: Implement real CSMA
                }
            }
        #endif
    }
}

// TODO: Remove this
void kiss_flushQueueDebug(void) {
    printf("Queue height %d\r\n", queue_height);
    for (size_t n = 0; n < queue_height; n++) {
        size_t start = fifo16_pop(&packet_starts);
        size_t length = fifo16_pop(&packet_lengths);

        printf("--- Packet %d, %d bytes ---\r\n", n+1, length);
        for (size_t i = 0; i < length; i++) {
            size_t pos = (start+i)%CONFIG_QUEUE_SIZE;
            printf("%02x", packet_queue[pos]);
        }
        printf("\r\n\r\n");
    }
    queue_height = 0;
    queued_bytes = 0;
}

volatile bool queue_flushing = false;
void kiss_flushQueue(void) {
    if (!queue_flushing) {
        queue_flushing = true;

        size_t processed = 0;
        for (size_t n = 0; n < queue_height; n++) {
            size_t start = fifo16_pop_locked(&packet_starts);
            size_t length = fifo16_pop_locked(&packet_lengths);

            //kiss_poll();
            for (size_t i = 0; i < length; i++) {
                size_t pos = (start+i)%CONFIG_QUEUE_SIZE;
                tx_buffer[i] = packet_queue[pos];
            }

            ax25_sendRaw(ax25ctx, tx_buffer, length);
            processed++;
        }

        if (processed < queue_height) {
            while (true) {
                LED_TX_ON();
                LED_RX_ON();
            }
        }
        //printf("Processed %d\r\n", processed);

        queue_height = 0;
        queued_bytes = 0;
        queue_flushing = false;
    }
}

uint8_t kiss_queuedPackets(void) {
    return 0;
}

bool kiss_queueIsFull(void) {
    return false;
}

void kiss_serialCallback(uint8_t sbyte) {
    if (IN_FRAME && sbyte == FEND && command == CMD_DATA) {
        IN_FRAME = false;

        if (queue_height < CONFIG_QUEUE_MAX_LENGTH && queued_bytes < CONFIG_QUEUE_SIZE) {
            queue_height++;
            size_t s = current_packet_start;
            size_t e = queue_cursor-1; if (e == -1) e = CONFIG_QUEUE_SIZE-1;
            size_t l = (s < e) ? e - s + 1 : CONFIG_QUEUE_SIZE - s + e + 1;

            fifo16_push_locked(&packet_starts, s);
            fifo16_push_locked(&packet_lengths, l);

            current_packet_start = queue_cursor;
            //printf("Queue height %d\r\n", queue_height);
        }
        
    } else if (sbyte == FEND) {
        IN_FRAME = true;
        command = CMD_UNKNOWN;
        frame_len = 0;
    } else if (IN_FRAME && frame_len < AX25_MAX_FRAME_LEN) {
        // Have a look at the command byte first
        if (frame_len == 0 && command == CMD_UNKNOWN) {
            // OpenModem supports only one HDLC port, so we
            // strip off the port nibble of the command byte
            sbyte = sbyte & 0x0F;
            command = sbyte;
            if (command == CMD_DATA) current_packet_start = queue_cursor;
        } else if (command == CMD_DATA) {
            if (sbyte == FESC) {
                ESCAPE = true;
            } else {
                if (ESCAPE) {
                    if (sbyte == TFEND) sbyte = FEND;
                    if (sbyte == TFESC) sbyte = FESC;
                    ESCAPE = false;
                }
                if (queue_height < CONFIG_QUEUE_MAX_LENGTH && queued_bytes < CONFIG_QUEUE_SIZE) {
                    queued_bytes++;
                    packet_queue[queue_cursor++] = sbyte;
                    if (queue_cursor == CONFIG_QUEUE_SIZE) queue_cursor = 0;
                }
            }
        } else if (command == CMD_TXDELAY) {
            custom_preamble = sbyte * 10UL;
        } else if (command == CMD_TXTAIL) {
            custom_tail = sbyte * 10;
        } else if (command == CMD_SLOTTIME) {
            slotTime = sbyte * 10;
        } else if (command == CMD_P) {
            p = sbyte;
        } else if (command == CMD_FLUSHQUEUE) {
            kiss_flushQueue();
        // TODO: Remove this
        } else if (command == CMD_FLUSHQUEUE_DEBUG) {
            kiss_flushQueueDebug();
        } else if (command == CMD_LED_INTENSITY) {
            if (sbyte == FESC) {
                ESCAPE = true;
            } else {
                if (ESCAPE) {
                    if (sbyte == TFEND) sbyte = FEND;
                    if (sbyte == TFESC) sbyte = FESC;
                    ESCAPE = false;
                }
                LED_setIntensity(sbyte);   
            }
        }
        
    }
}
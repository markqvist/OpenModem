#ifndef PROTOCOL_AX25_H
#define PROTOCOL_AX25_H

#include <stdio.h>
#include <stdbool.h>
#include "device.h"
#include "hardware/AFSK.h"

#define AX25_MIN_FRAME_LEN 18
#ifndef CUSTOM_FRAME_SIZE
    #define AX25_MAX_FRAME_LEN 792
#else
    #define AX25_MAX_FRAME_LEN CUSTOM_FRAME_SIZE
#endif

#define AX25_CRC_CORRECT  0xF0B8

#define AX25_CTRL_UI      0x03
#define AX25_PID_NOLAYER3 0xF0

struct AX25Ctx;     // Forward declarations
struct AX25Msg;

typedef void (*ax25_callback_t)(struct AX25Ctx *ctx);

typedef struct AX25Ctx {
    uint8_t buf[AX25_MAX_FRAME_LEN];
    Afsk *modem;
    FILE *ch;
    size_t frame_len;
    uint16_t crc_in;
    uint16_t crc_out;
    ax25_callback_t hook;
    bool sync;
    bool escape;
    bool ready_for_data;
} AX25Ctx;

void ax25_poll(AX25Ctx *ctx);
void ax25_sendRaw(AX25Ctx *ctx, void *_buf, size_t len);
void ax25_init(AX25Ctx *ctx, Afsk *modem, FILE *channel, ax25_callback_t hook);

#endif
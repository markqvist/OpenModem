#ifndef AFSK_H
#define AFSK_H

#include "device.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include "util/FIFO.h"
#include "protocol/HDLC.h"

#define SIN_LEN 512
static const uint8_t sine_table[] =
{
    128, 129, 131, 132, 134, 135, 137, 138, 140, 142, 143, 145, 146, 148, 149, 151,
    152, 154, 155, 157, 158, 160, 162, 163, 165, 166, 167, 169, 170, 172, 173, 175,
    176, 178, 179, 181, 182, 183, 185, 186, 188, 189, 190, 192, 193, 194, 196, 197,
    198, 200, 201, 202, 203, 205, 206, 207, 208, 210, 211, 212, 213, 214, 215, 217,
    218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233,
    234, 234, 235, 236, 237, 238, 238, 239, 240, 241, 241, 242, 243, 243, 244, 245,
    245, 246, 246, 247, 248, 248, 249, 249, 250, 250, 250, 251, 251, 252, 252, 252,
    253, 253, 253, 253, 254, 254, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255,
};

inline static uint8_t sinSample(uint16_t i) {
    uint16_t newI = i % (SIN_LEN/2);
    newI = (newI >= (SIN_LEN/4)) ? (SIN_LEN/2 - newI -1) : newI;
    uint8_t sine = sine_table[newI];
    return (i >= (SIN_LEN/2)) ? (255 - sine) : sine;
}


#define SWITCH_TONE(inc)  (((inc) == MARK_INC) ? SPACE_INC : MARK_INC)
#define BITS_DIFFER(bits1, bits2) (((bits1)^(bits2)) & 0x01)
#define TRANSITION_FOUND(bits) BITS_DIFFER((bits), (bits) >> 1)
#define DUAL_XOR(bits1, bits2) ((((bits1)^(bits2)) & 0x03) == 0x03)
#define QUAD_XOR(bits1, bits2) ((((bits1)^(bits2)) & 0x0F) == 0x0F)

#define CPU_FREQ F_CPU


#define BITRATE 1200

#if BITRATE == 300
    #define CONFIG_ADC_SAMPLERATE 9600UL
    #define CONFIG_DAC_SAMPLERATE 19200UL
    #define CLOCK_TICKS_PER_10_MS 96
#elif BITRATE == 1200
    #define CONFIG_ADC_SAMPLERATE 9600UL
    #define CONFIG_DAC_SAMPLERATE 19200UL
    #define CLOCK_TICKS_PER_10_MS 96
#elif BITRATE == 2400
    #define CONFIG_ADC_SAMPLERATE 19200UL
    #define CONFIG_DAC_SAMPLERATE 19200UL
    #define CLOCK_TICKS_PER_10_MS 192
#endif

#define CLOCK_TICKS_PER_SEC CONFIG_ADC_SAMPLERATE

#define CONFIG_AFSK_RX_BUFLEN AX25_MAX_FRAME_LEN
#define CONFIG_AFSK_TX_BUFLEN AX25_MAX_FRAME_LEN
#define CONFIG_AFSK_RXTIMEOUT 0

#if BITRATE == 300
    #define CONFIG_AFSK_PREAMBLE_LEN 150UL
    #define CONFIG_AFSK_TRAILER_LEN 10UL
#elif BITRATE == 1200
    #define CONFIG_AFSK_PREAMBLE_LEN 150UL
    #define CONFIG_AFSK_TRAILER_LEN 10UL
#elif BITRATE == 2400
    #define CONFIG_AFSK_PREAMBLE_LEN 200UL
    #define CONFIG_AFSK_TRAILER_LEN 25UL
#endif

#define BIT_STUFF_LEN 5

#define ADC_SAMPLESPERBIT (CONFIG_ADC_SAMPLERATE / BITRATE)
#define ADC_TICKS_BETWEEN_SAMPLES ((((CPU_FREQ+FREQUENCY_CORRECTION)) / CONFIG_ADC_SAMPLERATE) - 1)

#define DAC_SAMPLESPERBIT (CONFIG_DAC_SAMPLERATE / BITRATE)
#define DAC_TICKS_BETWEEN_SAMPLES ((((CPU_FREQ+FREQUENCY_CORRECTION)) / CONFIG_DAC_SAMPLERATE) - 1)

// TODO: Maybe revert to only looking at two samples
#if BITRATE == 300
    #define SIGNAL_TRANSITIONED(bits) DUAL_XOR((bits), (bits) >> 2)
#elif BITRATE == 1200
    #if CONFIG_ADC_SAMPLERATE == 19200
        #define SIGNAL_TRANSITIONED(bits) QUAD_XOR((bits), (bits) >> 4)
    #elif CONFIG_ADC_SAMPLERATE == 9600
        #define SIGNAL_TRANSITIONED(bits) DUAL_XOR((bits), (bits) >> 2)
    #endif
#elif BITRATE == 2400
    #define SIGNAL_TRANSITIONED(bits) DUAL_XOR((bits), (bits) >> 2)
#endif

#if BITRATE == 300
    // TODO: Real-world tests on which resolution is best
    //#define PHASE_BITS   8
    #define PHASE_BITS   4
#else
    #define PHASE_BITS   8 // Sub-sample phase counter resolution
#endif

#define PHASE_INC    1                              // Nudge by above resolution for each adjustment

#define PHASE_MAX    (ADC_SAMPLESPERBIT * PHASE_BITS)   // Size of our phase counter

// TODO: Test which target is best in real world
// For 1200, this seems a little better
#if BITRATE == 300
    #define PHASE_THRESHOLD  (PHASE_MAX / 2)
#elif BITRATE == 1200
    #if CONFIG_ADC_SAMPLERATE == 19200
        #define PHASE_THRESHOLD  (PHASE_MAX / 2)+3*PHASE_BITS  // Target transition point of our phase window
    #elif CONFIG_ADC_SAMPLERATE == 9600
        #define PHASE_THRESHOLD  (PHASE_MAX / 2)            // 64   // Target transition point of our phase window
    #endif
#elif BITRATE == 2400
    #define PHASE_THRESHOLD  (PHASE_MAX / 2)  // Target transition point of our phase window
#endif

#if BITRATE == 300
    #define DCD_TIMEOUT_SAMPLES 520
    #define DCD_MIN_COUNT 2
#elif BITRATE == 1200
    #define DCD_TIMEOUT_SAMPLES CONFIG_ADC_SAMPLERATE/100
    #define DCD_MIN_COUNT CONFIG_ADC_SAMPLERATE/1600
#elif BITRATE == 2400
    #define DCD_TIMEOUT_SAMPLES CONFIG_ADC_SAMPLERATE/100
    #define DCD_MIN_COUNT CONFIG_ADC_SAMPLERATE/1600
#endif
         
// TODO: Revamp filtering              
#if BITRATE == 1200
    #define FILTER_CUTOFF 500
    #define MARK_FREQ  1200
    #define SPACE_FREQ 2200
#elif BITRATE == 2400
    // TODO: Determine best filter cutoff
    // #define FILTER_CUTOFF 772
    #define FILTER_CUTOFF 1000
    #define MARK_FREQ  2165
    #define SPACE_FREQ 3970
    //#define MARK_FREQ  2200
    //#define SPACE_FREQ 4000
#elif BITRATE == 300
    #define FILTER_CUTOFF 500
    #define MARK_FREQ  1600
    #define SPACE_FREQ 1800
#else
    #error Unsupported bitrate!
#endif

typedef struct Hdlc
{
    uint8_t demodulatedBits;
    uint8_t bitIndex;
    uint8_t currentByte;
    bool receiving;
    bool dcd;
    uint8_t dcd_count;
} Hdlc;

typedef struct Afsk
{
    // Stream access to modem
    FILE fd;

    // General values
    Hdlc hdlc;                              // We need a link control structure
    uint16_t preambleLength;                // Length of sync preamble
    uint16_t tailLength;                    // Length of transmission tail

    // Modulation values
    uint8_t sampleIndex;                    // Current sample index for outgoing bit 
    uint8_t currentOutputByte;              // Current byte to be modulated
    uint8_t txBit;                          // Mask of current modulated bit
    bool bitStuff;                          // Whether bitstuffing is allowed

    uint8_t bitstuffCount;                  // Counter for bit-stuffing

    uint16_t phaseAcc;                      // Phase accumulator
    uint16_t phaseInc;                      // Phase increment per sample

    uint16_t silentSamples;                 // How many samples were completely silent

    volatile bool sending;                  // Set when modem is sending
    volatile bool sending_data;             // Set when modem is sending data

    // Demodulation values
    FIFOBuffer delayFifo;                   // Delayed FIFO for frequency discrimination
    #if BITRATE == 1200
        int8_t delayBuf[ADC_SAMPLESPERBIT / 2 + 1]; // Actual data storage for said FIFO
    #elif BITRATE == 2400
        int8_t delayBuf[7 + 1];
    #elif BITRATE == 300
        int8_t delayBuf[16 + 1];
    #endif

    FIFOBuffer rxFifo;                      // FIFO for received data
    uint8_t rxBuf[CONFIG_AFSK_RX_BUFLEN];   // Actual data storage for said FIFO

    FIFOBuffer txFifo;                      // FIFO for transmit data
    uint8_t txBuf[CONFIG_AFSK_TX_BUFLEN];   // Actual data storage for said FIFO

    int16_t iirX[2];                        // IIR Filter X cells
    int16_t iirY[2];                        // IIR Filter Y cells

    #if ADC_SAMPLESPERBIT < 17
        uint16_t sampledBits;               // Bits sampled by the demodulator (at ADC speed)
    #elif ADC_SAMPLESPERBIT < 33
        uint32_t sampledBits;
    #else
        #error Not enough space in sampledBits variable!
    #endif
    int16_t currentPhase;                    // Current phase of the demodulator
    uint8_t actualBits;                     // Actual found bits at correct bitrate

    volatile int status;                    // Status of the modem, 0 means OK

} Afsk;

#define DIV_ROUND(dividend, divisor)  (((dividend) + (divisor) / 2) / (divisor))
#define MARK_INC   (uint16_t)(DIV_ROUND(SIN_LEN * (uint32_t)MARK_FREQ, CONFIG_DAC_SAMPLERATE))
#define SPACE_INC  (uint16_t)(DIV_ROUND(SIN_LEN * (uint32_t)SPACE_FREQ, CONFIG_DAC_SAMPLERATE))

#define AFSK_DAC_IRQ_START()   do { extern bool hw_afsk_dac_isr; hw_afsk_dac_isr = true; } while (0)
#define AFSK_DAC_IRQ_STOP()    do { extern bool hw_afsk_dac_isr; hw_afsk_dac_isr = false; } while (0)

void AFSK_init(Afsk *afsk);
void AFSK_adc_init(void);
void AFSK_dac_init(void);
void AFSK_poll(Afsk *afsk);

#endif
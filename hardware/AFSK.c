#include <string.h>
#include "AFSK.h"
#include "util/time.h"
#include "protocol/KISS.h"

// TODO: Remove testing vars ////
#define SAMPLES_TO_CAPTURE 128
ticks_t capturedsamples = 0;
uint8_t samplebuf[SAMPLES_TO_CAPTURE];
/////////////////////////////////

extern volatile ticks_t _clock;
extern unsigned long custom_preamble;
extern unsigned long custom_tail;

bool hw_afsk_dac_isr = false;
bool hw_5v_ref = false;
Afsk *AFSK_modem;

// Forward declerations
int afsk_getchar(FILE *strem);
int afsk_putchar(char c, FILE *stream);

// ADC and clock setup
void AFSK_hw_init(void) {
    // Run ADC initialisation
    AFSK_adc_init();

    // Run DAC initialisation
    AFSK_dac_init();

    // Run LED initialisation
    LED_TX_INIT();
    LED_RX_INIT();
}

void AFSK_dac_init(void) {
    // DAC uses all 8 pins of one port,
    // so set all to output
    DAC_DDR |= 0xFF;

    // Set Timer3 to normal operation
    TCCR3A = 0;
    TCCR3B = _BV(CS10) |
             _BV(WGM33)|
             _BV(WGM32);

    ICR3 = DAC_TICKS_BETWEEN_SAMPLES;
    //OCR3A = DAC_TICKS_BETWEEN_SAMPLES;

    TIMSK3 = _BV(ICIE3);

}

void AFSK_adc_init(void) {
    // Set Timer1 to normal operation
    TCCR1A = 0;

    TCCR1B =    _BV(WGM13) |    // Enable Timer1 Waveform Generation Mode 12:
                _BV(WGM12) |    // Mode = CTC, TOP = ICR1
                _BV(CS10);      // Set clock source to 0b001 = System clock without prescaling

    // Set ICR1 register to the amount of ticks needed between
    // each sample capture/synthesis
    ICR1 = ADC_TICKS_BETWEEN_SAMPLES;

    // Set ADMUX register to use external AREF, channel ADC0
    // and left adjust result
    ADMUX = _BV(ADLAR) | 0;

    // Set ADC port directions and outputs
    // TODO: Check this
    ADC_DDR  &= ~_BV(0); // 0b11111110 - All pins are outputs, except ADC0
    ADC_PORT &= 0x00;    // 0b00000000 - All pins are at GND level

    // Set Digital Input Disable Register mask to 0b00000001,
    // which disables the input buffer on ADC0 pin to avoid
    // current through the pin.
    DIDR0 |= _BV(0);


    ADCSRB =    _BV(ADTS2) |    
                _BV(ADTS1) |
                _BV(ADTS0);     // Set ADC Trigger Source to 0b111 = Timer1 Capture Event

    ADCSRA =    _BV(ADEN) |     // ADC Enable
                _BV(ADSC) |     // ADC Start Conversion
                _BV(ADATE)|     // ADC Interrupt Flag
                _BV(ADIE) |     // ADC Interrupt Enable
                _BV(ADPS0)|
                _BV(ADPS2);     // Set ADC prescaler bits to 0b101 = 32
                                // At 20MHz, this gives an ADC clock of 625 KHz

}

void AFSK_init(Afsk *afsk) {
    // Allocate modem struct memory
    memset(afsk, 0, sizeof(*afsk));
    AFSK_modem = afsk;
    // Set phase increment
    afsk->phaseInc = MARK_INC;
    afsk->silentSamples = 0;

    // Initialise FIFO buffers
    fifo_init(&afsk->delayFifo, (uint8_t *)afsk->delayBuf, sizeof(afsk->delayBuf));
    fifo_init(&afsk->rxFifo, afsk->rxBuf, sizeof(afsk->rxBuf));
    fifo_init(&afsk->txFifo, afsk->txBuf, sizeof(afsk->txBuf));

    // Fill delay FIFO with zeroes
    for (int i = 0; i<ADC_SAMPLESPERBIT / 2; i++) {
        fifo_push(&afsk->delayFifo, 0);
    }

    AFSK_hw_init();

    // Set up streams
    FILE afsk_fd = FDEV_SETUP_STREAM(afsk_putchar, afsk_getchar, _FDEV_SETUP_RW);
    afsk->fd = afsk_fd;
}

static void AFSK_txStart(Afsk *afsk) {
    if (!afsk->sending) {
        afsk->phaseInc = MARK_INC;
        afsk->phaseAcc = 0;
        afsk->bitstuffCount = 0;
        afsk->sending = true;
        afsk->sending_data = true;
        LED_TX_ON();
        afsk->preambleLength = DIV_ROUND(custom_preamble * BITRATE, 8000);
        AFSK_DAC_IRQ_START();
    }
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      afsk->tailLength = DIV_ROUND(custom_tail * BITRATE, 8000);
    }
}

int afsk_putchar(char c, FILE *stream) {
    AFSK_txStart(AFSK_modem);
    while(fifo_isfull_locked(&AFSK_modem->txFifo)) { /* Wait */ }
    fifo_push_locked(&AFSK_modem->txFifo, c);
    return 1;
}

int afsk_getchar(FILE *stream) {
    if (fifo_isempty_locked(&AFSK_modem->rxFifo)) {
        return EOF;
    } else {
        return fifo_pop_locked(&AFSK_modem->rxFifo);
    }
}

void AFSK_transmit(char *buffer, size_t size) {
    fifo_flush(&AFSK_modem->txFifo);
    int i = 0;
    while (size--) {
        afsk_putchar(buffer[i++], NULL);
    }
}

uint8_t AFSK_dac_isr(Afsk *afsk) {
    if (afsk->sampleIndex == 0) {
        if (afsk->txBit == 0) {
            if (fifo_isempty(&afsk->txFifo) && afsk->tailLength == 0) {
                AFSK_DAC_IRQ_STOP();
                afsk->sending = false;
                afsk->sending_data = false;
                LED_TX_OFF();
                return 0;
            } else {
                if (!afsk->bitStuff) afsk->bitstuffCount = 0;
                afsk->bitStuff = true;
                if (afsk->preambleLength == 0) {
                    if (fifo_isempty(&afsk->txFifo)) {
                        afsk->sending_data = false;
                        afsk->tailLength--;
                        afsk->currentOutputByte = HDLC_FLAG;
                    } else {
                        afsk->currentOutputByte = fifo_pop(&afsk->txFifo);
                    }
                } else {
                    afsk->preambleLength--;
                    afsk->currentOutputByte = HDLC_FLAG;
                }
                if (afsk->currentOutputByte == AX25_ESC) {
                    if (fifo_isempty(&afsk->txFifo)) {
                        AFSK_DAC_IRQ_STOP();
                        afsk->sending = false;
                        LED_TX_OFF();
                        return 0;
                    } else {
                        afsk->currentOutputByte = fifo_pop(&afsk->txFifo);
                    }
                } else if (afsk->currentOutputByte == HDLC_FLAG || afsk->currentOutputByte == HDLC_RESET) {
                    afsk->bitStuff = false;
                }
            }
            afsk->txBit = 0x01;
        }

        if (afsk->bitStuff && afsk->bitstuffCount >= BIT_STUFF_LEN) {
            afsk->bitstuffCount = 0;
            afsk->phaseInc = SWITCH_TONE(afsk->phaseInc);
        } else {
            if (afsk->currentOutputByte & afsk->txBit) {
                afsk->bitstuffCount++;
            } else {
                afsk->bitstuffCount = 0;
                afsk->phaseInc = SWITCH_TONE(afsk->phaseInc);
            }
            afsk->txBit <<= 1;
        }

        afsk->sampleIndex = DAC_SAMPLESPERBIT;
    }

    afsk->phaseAcc += afsk->phaseInc;
    afsk->phaseAcc %= SIN_LEN;
    afsk->sampleIndex--;

    return sinSample(afsk->phaseAcc);
}

static bool hdlcParse(Hdlc *hdlc, bool bit, FIFOBuffer *fifo) {
    // Initialise a return value. We start with the
    // assumption that all is going to end well :)
    bool ret = true;

    // Bitshift our byte of demodulated bits to
    // the left by one bit, to make room for the
    // next incoming bit
    hdlc->demodulatedBits <<= 1;
    // And then put the newest bit from the 
    // demodulator into the byte.
    hdlc->demodulatedBits |= bit ? 1 : 0;

    // Now we'll look at the last 8 received bits, and
    // check if we have received a HDLC flag (01111110)
    if (hdlc->demodulatedBits == HDLC_FLAG) {
        // If we have, check that our output buffer is
        // not full.
        if (!fifo_isfull(fifo)) {
            // If it isn't, we'll push the HDLC_FLAG into
            // the buffer and indicate that we are now
            // receiving data. For bling we also turn
            // on the RX LED.
            fifo_push(fifo, HDLC_FLAG);
            hdlc->receiving = true;

            if (hdlc->dcd_count < DCD_MIN_COUNT) {
                hdlc->dcd = false;
                hdlc->dcd_count++;
            } else {
                hdlc->dcd = true;
            }

            #if OPEN_SQUELCH == false
                LED_RX_ON();
            #endif
        } else {
            // If the buffer is full, we have a problem
            // and abort by setting the return value to     
            // false and stopping the here.
            
            ret = false;
            hdlc->receiving = false;
            hdlc->dcd = false;
            hdlc->dcd_count = 0;
        }

        // Everytime we receive a HDLC_FLAG, we reset the
        // storage for our current incoming byte and bit
        // position in that byte. This effectively
        // synchronises our parsing to  the start and end
        // of the received bytes.
        hdlc->currentByte = 0;
        hdlc->bitIndex = 0;
        return ret;
    }

    // Check if we have received a RESET flag (01111111)
    // In this comparison we also detect when no transmission
    // (or silence) is taking place, and the demodulator
    // returns an endless stream of zeroes. Due to the NRZ-S
    // coding, the actual bits sent to this function will
    // be an endless stream of ones, which this AND operation
    // will also detect.
    if ((hdlc->demodulatedBits & HDLC_RESET) == HDLC_RESET) {
        // If we have, something probably went wrong at the
        // transmitting end, and we abort the reception.
        hdlc->receiving = false;
        hdlc->dcd = false;
        hdlc->dcd_count = 0;
        return ret;
    }

    // Check the DCD status and set RX LED appropriately
    if (hdlc->dcd) {
        LED_RX_ON();
    } else {
        LED_RX_OFF();
    }

    // If we have not yet seen a HDLC_FLAG indicating that
    // a transmission is actually taking place, don't bother
    // with anything.
    if (!hdlc->receiving) {
        hdlc->dcd = false;
        hdlc->dcd_count = 0;

        return ret;
    }

    // First check if what we are seeing is a stuffed bit.
    // Since the different HDLC control characters like
    // HDLC_FLAG, HDLC_RESET and such could also occur in
    // a normal data stream, we employ a method known as
    // "bit stuffing". All control characters have more than
    // 5 ones in a row, so if the transmitting party detects
    // this sequence in the _data_ to be transmitted, it inserts
    // a zero to avoid the receiving party interpreting it as
    // a control character. Therefore, if we detect such a
    // "stuffed bit", we simply ignore it and wait for the
    // next bit to come in.
    // 
    // We do the detection by applying an AND bit-mask to the
    // stream of demodulated bits. This mask is 00111111 (0x3f)
    // if the result of the operation is 00111110 (0x3e), we
    // have detected a stuffed bit.
    if ((hdlc->demodulatedBits & 0x3f) == 0x3e)
        return ret;

    // If we have an actual 1 bit, push this to the current byte
    // If it's a zero, we don't need to do anything, since the
    // bit is initialized to zero when we bitshifted earlier.
    if (hdlc->demodulatedBits & 0x01)
        hdlc->currentByte |= 0x80;

    // Increment the bitIndex and check if we have a complete byte
    if (++hdlc->bitIndex >= 8) {
        // If we have a HDLC control character, put a AX.25 escape
        // in the received data. We know we need to do this,
        // because at this point we must have already seen a HDLC
        // flag, meaning that this control character is the result
        // of a bitstuffed byte that is equal to said control
        // character, but is actually part of the data stream.
        // By inserting the escape character, we tell the protocol
        // layer that this is not an actual control character, but
        // data.
        if ((hdlc->currentByte == HDLC_FLAG ||
             hdlc->currentByte == HDLC_RESET ||
             hdlc->currentByte == AX25_ESC)) {
            // We also need to check that our received data buffer
            // is not full before putting more data in
            if (!fifo_isfull(fifo)) {
                fifo_push(fifo, AX25_ESC);
            } else {
                // If it is, abort and return false
                hdlc->receiving = false;
                hdlc->dcd = false;
                hdlc->dcd_count = 0;
                LED_RX_OFF();
                ret = false;
            }
        }

        // Push the actual byte to the received data FIFO,
        // if it isn't full.
        if (!fifo_isfull(fifo)) {
            fifo_push(fifo, hdlc->currentByte);
        } else {
            // If it is, well, you know by now!
            hdlc->receiving = false;
            hdlc->dcd = false;
            hdlc->dcd_count = 0;
            LED_RX_OFF();
            ret = false;
        }

        // Wipe received byte and reset bit index to 0
        hdlc->currentByte = 0;
        hdlc->bitIndex = 0;

    } else {
        // We don't have a full byte yet, bitshift the byte
        // to make room for the next bit
        hdlc->currentByte >>= 1;
    }

    return ret;
}


void AFSK_adc_isr(Afsk *afsk, int8_t currentSample) {
    // To determine the received frequency, and thereby
    // the bit of the sample, we multiply the sample by
    // a sample delayed by (samples per bit / 2).
    // We then lowpass-filter the samples with a
    // Chebyshev filter. The lowpass filtering serves
    // to "smooth out" the variations in the samples.

    afsk->iirX[0] = afsk->iirX[1];

    #if CONFIG_ADC_SAMPLERATE == 4800
        #if FILTER_CUTOFF == 600
            #define IIR_GAIN 2 // Really 2.228465666
            #define IIR_POLE 10 // Really Y[0] * 0.1025215106
            afsk->iirX[1] = ((int8_t)fifo_pop(&afsk->delayFifo) * currentSample) / IIR_GAIN;
            afsk->iirY[0] = afsk->iirY[1];
            afsk->iirY[1] = afsk->iirX[0] + afsk->iirX[1] + (afsk->iirY[0] / IIR_POLE);

        #else
            #error Unsupported filter cutoff!
        #endif

    #elif CONFIG_ADC_SAMPLERATE == 9600
        #if FILTER_CUTOFF == 500
            #define IIR_GAIN 4 // Really 4.082041675
            #define IIR_POLE 2 // Really Y[0] * 0.5100490981
            afsk->iirX[1] = ((int8_t)fifo_pop(&afsk->delayFifo) * currentSample) / IIR_GAIN;
            afsk->iirY[0] = afsk->iirY[1];
            afsk->iirY[1] = afsk->iirX[0] + afsk->iirX[1] + (afsk->iirY[0] / IIR_POLE);

        #else
            #error Unsupported filter cutoff!
        #endif

    #elif CONFIG_ADC_SAMPLERATE == 19200
        #if FILTER_CUTOFF == 150
            #define IIR_GAIN 2 // Really 2.172813446e
            #define IIR_POLE 2 // Really Y[0] * 0.9079534415
            afsk->iirX[1] = ((int8_t)fifo_pop(&afsk->delayFifo) * currentSample) / IIR_GAIN;
            afsk->iirY[0] = afsk->iirY[1];
            afsk->iirY[1] = afsk->iirX[0] + afsk->iirX[1] + (afsk->iirY[0] / IIR_POLE); 

        #elif FILTER_CUTOFF == 500
            #define IIR_GAIN 7 // Really 5.006847792
            #define IIR_POLE 2 // Really Y[0] * 0.6005470741
            afsk->iirX[1] = ((int8_t)fifo_pop(&afsk->delayFifo) * currentSample) / IIR_GAIN;
            afsk->iirY[0] = afsk->iirY[1];
            afsk->iirY[1] = afsk->iirX[0] + afsk->iirX[1] + (afsk->iirY[0] / IIR_POLE); 

        #elif FILTER_CUTOFF == 600
            #define IIR_GAIN 6 // Really 6.166411713
            #define IIR_POLE 2 // Really Y[0] * 0.6756622663
            afsk->iirX[1] = ((int8_t)fifo_pop(&afsk->delayFifo) * currentSample) / IIR_GAIN;
            afsk->iirY[0] = afsk->iirY[1];
            afsk->iirY[1] = afsk->iirX[0] + afsk->iirX[1] + (afsk->iirY[0] / IIR_POLE); 

        #elif FILTER_CUTOFF == 772
            #define IIR_GAIN 5 // Really 5.006847792
            #define IIR_POLE 2 // Really Y[0] * 0.6005470741
            afsk->iirX[1] = ((int8_t)fifo_pop(&afsk->delayFifo) * currentSample) / IIR_GAIN;
            afsk->iirY[0] = afsk->iirY[1];
            afsk->iirY[1] = afsk->iirX[0] + afsk->iirX[1] + (afsk->iirY[0] / IIR_POLE);  

        #elif FILTER_CUTOFF == 1000
            #define IIR_GAIN 4 // Really 4.082041675
            #define IIR_POLE 2 // Really Y[0] * 0.5100490981
            afsk->iirX[1] = ((int8_t)fifo_pop(&afsk->delayFifo) * currentSample) / IIR_GAIN;
            afsk->iirY[0] = afsk->iirY[1];
            afsk->iirY[1] = afsk->iirX[0] + afsk->iirX[1] + (afsk->iirY[0] / IIR_POLE);

        #elif FILTER_CUTOFF == 1400
            #define IIR_GAIN 3 // Really 3.182326364
            #define IIR_POLE 3 // Really Y[0] * 0.3715289474
            afsk->iirX[1] = ((int8_t)fifo_pop(&afsk->delayFifo) * currentSample) / IIR_GAIN;
            afsk->iirY[0] = afsk->iirY[1];
            afsk->iirY[1] = afsk->iirX[0] + afsk->iirX[1] + (afsk->iirY[0] / IIR_POLE);    

        #else
            #error Unsupported filter cutoff!
        #endif
    #else
        #error No filters defined for specified samplerate!
    #endif

    // We put the sampled bit in a delay-line:
    // First we bitshift everything 1 left
    afsk->sampledBits <<= 1;

    // And then add the sampled bit to our delay line
    afsk->sampledBits |= (afsk->iirY[1] > 0) ? 0 : 1;
    //afsk->sampledBits |= (freq_disc > 0) ? 0 : 1;

    // Put the current raw sample in the delay FIFO
    fifo_push(&afsk->delayFifo, currentSample);

    // We need to check whether there is a signal transition.
    // If there is, we can recalibrate the phase of our 
    // sampler to stay in sync with the transmitter. A bit of
    // explanation is required to understand how this works.
    // Since we have PHASE_MAX/PHASE_BITS = 8 samples per bit,
    // we employ a phase counter (currentPhase), that increments
    // by PHASE_BITS everytime a sample is captured. When this
    // counter reaches PHASE_MAX, it wraps around by modulus
    // PHASE_MAX. We then look at the last three samples we
    // captured and determine if the bit was a one or a zero.
    //
    // This gives us a "window" looking into the stream of
    // samples coming from the ADC. Sort of like this:
    //
    //   Past                                      Future
    //       0000000011111111000000001111111100000000
    //                   |________|
    //                       ||     
    //                     Window
    //
    // Every time we detect a signal transition, we adjust
    // where this window is positioned a little. How much we
    // adjust it is defined by PHASE_INC. If our current phase
    // phase counter value is less than half of PHASE_MAX (ie, 
    // the window size) when a signal transition is detected,
    // add PHASE_INC to our phase counter, effectively moving
    // the window a little bit backward (to the left in the
    // illustration), inversely, if the phase counter is greater
    // than half of PHASE_MAX, we move it forward a little.
    // This way, our "window" is constantly seeking to position
    // it's center at the bit transitions. Thus, we synchronise
    // our timing to the transmitter, even if it's timing is
    // a little off compared to our own.
    if (SIGNAL_TRANSITIONED(afsk->sampledBits)) {
        if (afsk->currentPhase < PHASE_THRESHOLD) {
            afsk->currentPhase += PHASE_INC;
        } else {
            afsk->currentPhase -= PHASE_INC;
        }
        afsk->silentSamples = 0;
    } else {
        afsk->silentSamples++;
    }

    // We increment our phase counter
    afsk->currentPhase += PHASE_BITS;

    // Check if we have reached the end of
    // our sampling window.
    if (afsk->currentPhase >= PHASE_MAX) {
        // If we have, wrap around our phase
        // counter by modulus
        afsk->currentPhase %= PHASE_MAX;

        // Bitshift to make room for the next
        // bit in our stream of demodulated bits
        afsk->actualBits <<= 1;

        // We determine the actual bit value by reading
        // the last 3 sampled bits. If there is two or
        // more 1's, we will assume that the transmitter
        // sent us a one, otherwise we assume a zero
        
        uint8_t bits = afsk->sampledBits & 0x07;
        if (bits == 0x07 || // 111
            bits == 0x06 || // 110
            bits == 0x05 || // 101
            bits == 0x03    // 011
            ) {
            afsk->actualBits |= 1;
        }


        //// Alternative using six bits ////////////////
        // uint8_t bits = afsk->sampledBits & 0x3F;
        // uint8_t c = 0;
        // c += bits & _BV(0);
        // c += bits & _BV(1);
        // c += bits & _BV(2);
        // c += bits & _BV(3);
        // c += bits & _BV(4);
        // c += bits & _BV(5);
        // if (c >= 3) afsk->actualBits |= 1;
        /////////////////////////////////////////////////

        // Now we can pass the actual bit to the HDLC parser.
        // We are using NRZ-S coding, so if 2 consecutive bits
        // have the same value, we have a 1, otherwise a 0.
        // We use the TRANSITION_FOUND function to determine this.
        //
        // This is smart in combination with bit stuffing,
        // since it ensures a transmitter will never send more
        // than five consecutive 1's. When sending consecutive
        // ones, the signal stays at the same level, and if
        // this happens for longer periods of time, we would
        // not be able to synchronize our phase to the transmitter
        // and would start experiencing "bit slip".
        //
        // By combining bit-stuffing with NRZ-S coding, we ensure
        // that the signal will regularly make transitions
        // that we can use to synchronize our phase.
        //
        // We also check the return of the Link Control parser
        // to check if an error occured.

        if (!hdlcParse(&afsk->hdlc, !TRANSITION_FOUND(afsk->actualBits), &afsk->rxFifo)) {
            afsk->status |= 1;
            if (fifo_isfull(&afsk->rxFifo)) {
                fifo_flush(&afsk->rxFifo);
                afsk->status = 0;
            }
        }
    }

    if (afsk->silentSamples > DCD_TIMEOUT_SAMPLES) {
        afsk->silentSamples = 0;
        afsk->hdlc.dcd = false;
        LED_RX_OFF();
    }

}

ISR(TIMER3_CAPT_vect) {
    if (hw_afsk_dac_isr) {
        DAC_PORT = AFSK_dac_isr(AFSK_modem);
        LED_TX_ON();
    } else {
        LED_TX_OFF();
        DAC_PORT = 127;
    }
}

ISR(ADC_vect) {
    TIFR1 = _BV(ICF1);
    
    if (CONFIG_FULL_DUPLEX || !hw_afsk_dac_isr) {
        AFSK_adc_isr(AFSK_modem, (ADCH - 128));
    }

    ++_clock;
}
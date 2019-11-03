#include <stdlib.h>
#include <string.h>

#include "device.h"
#include "hardware/Serial.h"
#include "hardware/LED.h"
#include "hardware/Crypto.h"
#include "util/FIFO16.h"
#include "util/time.h"
#include "util/Config.h"
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
extern volatile int8_t afsk_peak;

size_t frame_len;
bool IN_FRAME;
bool ESCAPE;

uint8_t command = CMD_UNKNOWN;

bool log_ready = false;
uint32_t log_index = 0;
FIL log_fp;
char log_fb[4];
char log_filename[32];
FRESULT log_fr;
FILINFO log_fi;

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
    while (!fifo_isempty_locked(&uart0FIFO)) {
        char sbyte = fifo_pop_locked(&uart0FIFO);
        kiss_serialCallback(sbyte);
        last_serial_read = timer_clock();
    }
}

#if CONFIG_BENCHMARK_MODE
    size_t decodes = 0;
#endif
void kiss_messageCallback(AX25Ctx *ctx) {
    #if CONFIG_BENCHMARK_MODE
        decodes++;
        printf("%d\r\n", decodes);
    #else
        bool integrity_ok = false;
        if (crypto_enabled()) {
            size_t rxpos = 0;
            if (ctx->frame_len >= AX25_ENCRYPTED_MIN_LENGTH) {
                // Get padding size
                uint8_t padding = ctx->buf[rxpos++];
                size_t data_length = ctx->frame_len - 2 - 1 - CRYPTO_HMAC_SIZE - CRYPTO_KEY_SIZE;
                size_t hmac_offset = ctx->frame_len - 2 - CRYPTO_HMAC_SIZE;

                // Get HMAC
                uint8_t hmac[CRYPTO_HMAC_SIZE];
                memset(hmac, 0x00, CRYPTO_HMAC_SIZE);
                for (uint8_t i = 0; i < CRYPTO_HMAC_SIZE; i++) {
                    size_t pos = hmac_offset + i;
                    hmac[i] = ctx->buf[pos];
                }

                // Calculate HMAC
                crypto_generate_hmac(ctx->buf, ctx->frame_len-2-CRYPTO_HMAC_SIZE);
                bool HMAC_ok = true;
                for (uint8_t i = 0; i < CRYPTO_HMAC_SIZE; i++) {
                    if (hmac[i] != crypto_work_block[i]) {
                        HMAC_ok = false;
                        break;
                    }
                }

                if (HMAC_ok) {
                    // Get IV
                    for (uint8_t i = 0; i < CRYPTO_KEY_SIZE; i++) {
                        crypto_work_block[i] = ctx->buf[rxpos++];
                    }
                    crypto_set_iv_from_workblock();
                    
                    crypto_prepare();

                    uint8_t blocks = data_length / CRYPTO_KEY_SIZE;
                    size_t decrypted_pos = 0;
                    for (uint8_t block = 0; block < blocks; block++) {

                        for (uint8_t i = 0; i < CRYPTO_KEY_SIZE; i++) {
                            crypto_work_block[i] = ctx->buf[rxpos++];
                        }

                        crypto_decrypt_block(crypto_work_block);
                        
                        for (uint8_t i = 0; i < CRYPTO_KEY_SIZE; i++) {
                            ctx->buf[decrypted_pos++] = crypto_work_block[i];
                        }
                    }
                    ctx->frame_len = data_length - padding + 2;
                    integrity_ok = true;

                }
            }
        } else {
            integrity_ok = true;
        }

        if (integrity_ok) {
            bool log_write_ok = false;
            if (config_log_packets && sd_mounted()) {
                if (log_ready || log_init()) {
                    // TODO: Assertion here on max path length
                    memset(log_filename, 0x00, sizeof(log_filename));
                    snprintf(log_filename, sizeof(log_filename), "%s/%lu.pkt", PATH_LOG, log_index);
                    
                    // Open file descriptor
                    log_fr = f_open(&log_fp, log_filename, FA_CREATE_NEW | FA_WRITE);
                    if (log_fr == FR_OK) {
                        log_write_ok = true;
                    }
                }
            }

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
                if (log_write_ok) {
                    UINT written = 0;
                    log_fr = f_write(&log_fp, &b, 1, &written);
                }
            }
            fputc(FEND, &serial->uart0);

            if (config_log_packets && sd_mounted() && log_ready) {
                f_close(&log_fp);
                update_log_index();
            }
        }
    #endif
}

bool log_init(void) {
    if (sd_mounted()) {
        // Check that base dir exists
        log_fr = f_stat(PATH_BASE, &log_fi);
        if (log_fr == FR_NO_PATH || log_fr == FR_NO_FILE) {
            log_fr = f_mkdir(PATH_BASE);
            if (log_fr != FR_OK) {
                return false;
            } else {
            }
        }

        // Check that log dir exists
        log_fr = f_stat(PATH_LOG, &log_fi);
        if (log_fr == FR_NO_PATH || log_fr == FR_NO_FILE) {
            log_fr = f_mkdir(PATH_LOG);
            if (log_fr != FR_OK) {
                return false;
            }
        }

        if (load_log_index()) {
            log_ready = true;
            return true;
        } else {
            return false;
        }
        
    } else {
        return false;
    }
}

bool load_log_index(void) {
    if (sd_mounted()) {
        log_fr = f_open(&log_fp, PATH_LOG_INDEX, FA_READ);
        if (log_fr == FR_NO_FILE) {
            f_close(&log_fp);
            log_fr = f_open(&log_fp, PATH_LOG_INDEX, FA_CREATE_NEW | FA_WRITE);

            if (log_fr == FR_OK) {
                log_index = 0x00000000;
                memcpy(log_fb, &log_index, sizeof(log_index));
                
                UINT written = 0;
                log_fr = f_write(&log_fp, log_fb, sizeof(log_index), &written);
                f_close(&log_fp);
            }

            log_fr = f_open(&log_fp, PATH_LOG_INDEX, FA_READ);
        }

        if (log_fr == FR_OK) {
            UINT read = 0;
            log_fr = f_read(&log_fp, log_fb, sizeof(log_index), &read);
            f_close(&log_fp);
            if (log_fr == FR_OK && read == sizeof(log_index)) {
                memcpy(&log_index, log_fb, sizeof(log_index));
                return true;
            }
        }
    }

    f_close(&log_fp);
    return false;
}

bool update_log_index(void) {
    log_fr = f_open(&log_fp, PATH_LOG_INDEX, FA_WRITE);
    if (log_fr == FR_OK) {
        log_index++;
        memcpy(log_fb, &log_index, sizeof(log_index));

        UINT written = 0;
        log_fr = f_write(&log_fp, log_fb, sizeof(log_index), &written);

        f_close(&log_fp);

        if (log_fr == FR_OK && written == sizeof(log_index)) {
            return true;
        }
    }
    return false;
}

void kiss_csma(void) {
    if (queue_height > 0) {
        #if BITRATE == 2400
            if (!channel->hdlc.dcd) {
                ticks_t timeout = last_serial_read + ms_to_ticks(CONFIG_SERIAL_TIMEOUT_MS);
                if (timer_clock() > timeout) {
                    if (config_p == 255) {
                        kiss_flushQueue();
                    } else {
                        // TODO: Implement real CSMA
                        kiss_flushQueue();
                    }
                }
            }
        #else
            if (!channel->hdlc.dcd) {
                if (config_p == 255) {
                    kiss_flushQueue();
                } else {
                    // TODO: Implement real CSMA
                    kiss_flushQueue();
                }
            }
        #endif
    }
}

volatile bool queue_flushing = false;
void kiss_flushQueue(void) {
    if (!queue_flushing) {
        queue_flushing = true;

        size_t processed = 0;
        for (size_t n = 0; n < queue_height; n++) {
            size_t start = fifo16_pop_locked(&packet_starts);
            size_t length = fifo16_pop_locked(&packet_lengths);

            if (length >= AX25_MIN_PAYLOAD) {
                if (crypto_enabled()) {
                    uint8_t padding = CRYPTO_KEY_SIZE - (length % CRYPTO_KEY_SIZE);
                    if (padding == CRYPTO_KEY_SIZE) padding = 0;

                    uint8_t blocks = (length + padding) / CRYPTO_KEY_SIZE;
                    
                    if (crypto_generate_iv()) {
                        crypto_prepare();

                        size_t tx_pos = 0;
                        tx_buffer[tx_pos++] = padding;

                        uint8_t *iv = crypto_get_iv();
                        for (uint8_t i = 0; i < CRYPTO_KEY_SIZE; i++) {
                            tx_buffer[tx_pos++] = iv[i];
                        }

                        // Encrypt each block
                        for (uint8_t i = 0; i < blocks; i++) {
                            if (i < blocks-1 || padding == 0) {
                                for (uint8_t j = 0; j < CRYPTO_KEY_SIZE; j++) {
                                    size_t pos = (start+j)%CONFIG_QUEUE_SIZE;
                                    crypto_work_block[j] = packet_queue[pos];
                                }
                                start += CRYPTO_KEY_SIZE;
                            } else {
                                for (uint8_t j = 0; j < CRYPTO_KEY_SIZE - padding; j++) {
                                    size_t pos = (start+j)%CONFIG_QUEUE_SIZE;
                                    crypto_work_block[j] = packet_queue[pos];
                                }

                                for (uint8_t j = CRYPTO_KEY_SIZE - padding; j < CRYPTO_KEY_SIZE; j++) {
                                    crypto_work_block[j] = 0xFF;
                                }
                            }
                            
                            crypto_encrypt_block(crypto_work_block);

                            for (uint8_t j = 0; j < CRYPTO_KEY_SIZE; j++) {
                                tx_buffer[tx_pos++] = crypto_work_block[j];
                            }
                        }

                        // Genereate MAC
                        crypto_generate_hmac(tx_buffer, tx_pos);
                        for (uint8_t i = 0; i < CRYPTO_HMAC_SIZE; i++) {
                            tx_buffer[tx_pos++] = crypto_work_block[i];
                        }

                        // Check size and send
                        if (tx_pos <= AX25_MAX_FRAME_LEN) {
                            ax25_sendRaw(ax25ctx, tx_buffer, tx_pos);
                            processed++;
                        } else {
                            processed++;
                        }

                    } else {
                        LED_indicate_error_crypto();
                    }
                } else {
                    for (size_t i = 0; i < length; i++) {
                        size_t pos = (start+i)%CONFIG_QUEUE_SIZE;
                        tx_buffer[i] = packet_queue[pos];
                    }

                    ax25_sendRaw(ax25ctx, tx_buffer, length);
                    processed++;
                }
            }
        }

        if (processed < queue_height) {
            while (true) {
                LED_TX_ON();
                LED_RX_ON();
            }
        }

        queue_height = 0;
        queued_bytes = 0;
        queue_flushing = false;
    }
}

void kiss_serialCallback(uint8_t sbyte) {
    if (IN_FRAME && sbyte == FEND && command == CMD_DATA) {
        IN_FRAME = false;

        if (queue_height < CONFIG_QUEUE_MAX_LENGTH && queued_bytes < CONFIG_QUEUE_SIZE) {
            size_t s = current_packet_start;
            size_t e = queue_cursor-1; if (e == -1) e = CONFIG_QUEUE_SIZE-1;
            size_t l;

            if (s != e) {
                l = (s < e) ? e - s + 1 : CONFIG_QUEUE_SIZE - s + e + 1;
            } else {
                l = 1;
            }

            if (l >= AX25_MIN_PAYLOAD) {
                queue_height++;

                fifo16_push_locked(&packet_starts, s);
                fifo16_push_locked(&packet_lengths, l);

                current_packet_start = queue_cursor;
            }
        }
        
    } else if (sbyte == FEND) {
        IN_FRAME = true;
        command = CMD_UNKNOWN;
        frame_len = 0;
    } else if (IN_FRAME && frame_len < AX25_MAX_PAYLOAD) {
        // Have a look at the command byte first
        if (frame_len == 0 && command == CMD_UNKNOWN) {
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
        } else if (command == CMD_PREAMBLE) {
            config_preamble = sbyte * 10UL;
        } else if (command == CMD_TXTAIL) {
            config_tail = sbyte * 10UL;
        } else if (command == CMD_SLOTTIME) {
            config_slottime = sbyte * 10UL;
        } else if (command == CMD_P) {
            config_p = sbyte;
        } else if (command == CMD_SAVE_CONFIG) {
            config_save();
        } else if (command == CMD_REBOOT) {
            if (sbyte == CMD_REBOOT_CONFIRM) {
                config_soft_reboot();
            }
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
        } else if (command == CMD_OUTPUT_GAIN) {
            if (sbyte == FESC) { ESCAPE = true; } else {
                if (ESCAPE) {
                    if (sbyte == TFEND) sbyte = FEND;
                    if (sbyte == TFESC) sbyte = FESC;
                    ESCAPE = false;
                }
                config_set_output_gain(sbyte);
            }
        } else if (command == CMD_INPUT_GAIN) {
            if (sbyte == FESC) { ESCAPE = true; } else {
                if (ESCAPE) {
                    if (sbyte == TFEND) sbyte = FEND;
                    if (sbyte == TFESC) sbyte = FESC;
                    ESCAPE = false;
                }
                config_set_input_gain(sbyte);
            }
        } else if (command == CMD_PASSALL) {
            if (sbyte == FESC) { ESCAPE = true; } else {
                if (ESCAPE) {
                    if (sbyte == TFEND) sbyte = FEND;
                    if (sbyte == TFESC) sbyte = FESC;
                    ESCAPE = false;
                }
                config_set_passall(sbyte);
            }
        } else if (command == CMD_LOG_PACKETS) {
            if (sbyte == FESC) { ESCAPE = true; } else {
                if (ESCAPE) {
                    if (sbyte == TFEND) sbyte = FEND;
                    if (sbyte == TFESC) sbyte = FESC;
                    ESCAPE = false;
                }
                config_set_log_packets(sbyte);
            }
        } else if (command == CMD_GPS_MODE) {
            if (sbyte == FESC) { ESCAPE = true; } else {
                if (ESCAPE) {
                    if (sbyte == TFEND) sbyte = FEND;
                    if (sbyte == TFESC) sbyte = FESC;
                    ESCAPE = false;
                }
                config_set_gps_mode(sbyte);
            }
        } else if (command == CMD_BT_MODE) {
            if (sbyte == FESC) { ESCAPE = true; } else {
                if (ESCAPE) {
                    if (sbyte == TFEND) sbyte = FEND;
                    if (sbyte == TFESC) sbyte = FESC;
                    ESCAPE = false;
                }
                config_set_bt_mode(sbyte);
            }
        } else if (command == CMD_SERIAL_BAUDRATE) {
            config_set_serial_baudrate(sbyte);
        // TODO: Remove this
        } else if (command == CMD_PRINT_CONFIG) {
            config_print();
            kiss_output_modem_mode();
        } else if (command == CMD_AUDIO_PEAK) {
            if (sbyte == 0x01) {
                kiss_output_afsk_peak();
            }
        }  else if (command == CMD_ENABLE_DIAGNOSTICS) {
            if (sbyte == 0x00) {
                config_disable_diagnostics();
            } else {
                config_enable_diagnostics();
            }
        }  else if (command == CMD_MODE) {
            if (sbyte == 0x00) {
                kiss_output_modem_mode();
            }
        }
        
    }
}

void kiss_output_modem_mode(void) {
    fputc(FEND, &serial->uart0);
    fputc(CMD_MODE, &serial->uart0);
    if (BITRATE == 300) {
        fputc(MODE_AFSK_300, &serial->uart0);
    } else if (BITRATE == 1200) {
        fputc(MODE_AFSK_1200, &serial->uart0);
    } else if (BITRATE == 2400) {
        fputc(MODE_AFSK_2400, &serial->uart0);
    }
    fputc(FEND, &serial->uart0);
}

void kiss_output_afsk_peak(void) {
    fputc(FEND, &serial->uart0);
    fputc(CMD_AUDIO_PEAK, &serial->uart0);
    uint8_t b = afsk_peak;

    if (b == FEND) {
        fputc(FESC, &serial->uart0);
        fputc(TFEND, &serial->uart0);
    } else if (b == FESC) {
        fputc(FESC, &serial->uart0);
        fputc(TFESC, &serial->uart0);
    } else {
        fputc(b, &serial->uart0);
    }

    fputc(FEND, &serial->uart0);
}

void kiss_output_config(uint8_t* data, size_t length) {
    fputc(FEND, &serial->uart0);
    fputc(CMD_PRINT_CONFIG, &serial->uart0);
    for (unsigned i = 0; i < length; i++) {
        uint8_t b = data[i];
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
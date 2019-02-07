#include "SD.h"
#include "hardware/Crypto.h"

FATFS sdfs;

uint8_t sd_scheduler_timer = 0;
uint8_t sd_status = SD_STATUS_UNKNOWN;
bool sd_mountstate = false;

// Job flags
bool sd_flag_needs_automount = false;
bool sd_flag_needs_autounmount = false;
bool sd_flag_was_automounted = false;
bool sd_flag_was_autounmounted = false;

void sd_init(void) {
    SPI_DDR |= _BV(SPI_MOSI) | _BV(SPI_CLK);
    SPI_DDR &= ~(_BV(SPI_MISO));
    SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);

    SD_CS_DDR |= _BV(SD_CS_PIN);
    SD_DETECT_DDR &= ~_BV(SD_DETECT_PIN);
    SD_DETECT_PORT |= _BV(SD_DETECT_PIN); // Enable pull-up
}

void sd_automounted_hook(void) {
    sd_statuschange_indication(1);
    crypto_init();
}

void sd_autounmounted_hook(void) {
    sd_statuschange_indication(0);
}

void sd_jobs(void) {
    if (sd_flag_needs_automount) {
        sd_flag_needs_automount = false;
        sd_automount();
    }

    if (sd_flag_needs_autounmount) {
        sd_flag_needs_autounmount = false;
        sd_autounmount();
    }

    if (sd_flag_was_automounted) {
        sd_flag_was_automounted = false;
        sd_automounted_hook();
    }

    if (sd_flag_was_autounmounted) {
        sd_flag_was_autounmounted = false;
        sd_autounmounted_hook();
    }
}

void sd_statuschange_indication(uint8_t pattern) {
    if (pattern == 1) {
        for (uint8_t i = 0; i < 4; i++) {
            LED_STATUS_OFF();
            delay_ms(65);
            LED_STATUS_ON();
            delay_ms(30);
        }
    }
    
    if (pattern == 0) {
        for (uint8_t i = 0; i < 2; i++) {
            LED_STATUS_OFF();
            delay_ms(250);
            LED_STATUS_ON();
            delay_ms(20);
        }
    }
    
}

bool sd_inserted(void) {
    if (sd_status & SD_STATUS_NODISK) {
        return false;
    } else {
        return true;
    }

}

bool sd_card_ready(void) {
    if (sd_status == SD_STATUS_READY) {
        return true;
    } else {
        return false;
    }
}

bool sd_mounted(void) {
    return sd_mountstate;
}

bool sd_mount(void) {
    if (sd_inserted()) {
        FRESULT mount_result = 0xFF;
        mount_result = f_mount(&sdfs, "", 1);

        if (mount_result == 0) {
            sd_mountstate = true;
            
        } else {
            sd_mountstate = false;
        }

        return sd_mountstate;
    } else {
        return false;
    }
}

bool sd_unmount(void) {
    FRESULT unmount_result = 0xFF;
    unmount_result = f_mount(0, "", 0);

    if (unmount_result == 0) {
        sd_mountstate = false;
        return true;
    } else {
        return false;
    }
}

void sd_automount(void) {
    if (sd_mount()) {
        sd_automounted_hook();
    }
}

void sd_autounmount(void) {
    if (sd_unmount()) {
        sd_autounmounted_hook();
    }
}

void sd_scheduler(void) {
    sd_scheduler_timer++;

    disk_timerproc();
    sd_status = disk_status(0);
    
    if (sd_scheduler_timer % 50 == 0) {
        sd_scheduler_timer = 0;

        if (!sd_mounted() && sd_inserted()) {
            sd_flag_needs_automount = true;
        }

        if (!sd_inserted() && sd_mounted()) {
            sd_flag_needs_autounmount = true;
        }
    }
}

// TODO: Get time from RTC or host
DWORD get_fattime (void)
{
    // Returns current time packed into a DWORD variable 
    return    ((DWORD)(2018 - 1980) << 25)  // Year 2013 
    | ((DWORD)8 << 21)              // Month 7 
    | ((DWORD)2 << 16)              // Mday 28 
    | ((DWORD)20 << 11)             // Hour 0..24
    | ((DWORD)30 << 5)              // Min 0 
    | ((DWORD)0 >> 1);              // Sec 0
}
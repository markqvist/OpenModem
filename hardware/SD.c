#include "SD.h"

FATFS sdfs;    // FatFs work area 

void sd_init(void) {
    SPI_DDR |= _BV(SPI_MOSI) | _BV(SPI_CLK);
    SPI_DDR &= ~(_BV(SPI_MISO));
    SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);

    SD_CS_DDR |= _BV(SD_CS_PIN);
    SD_DETECT_DDR &= ~_BV(SD_DETECT_PIN);
    SD_DETECT_PORT |= _BV(SD_DETECT_PIN); // Enable pull-up
}

// TODO: Remove this
void sd_test(void) {
    printf("Testing SD card functions\r\n");

    FRESULT res = 0xFF;
    while (res != 0) {
        res = f_mount(&sdfs, "", 1);
        printf("SD Detect: %d\r\n", (SD_DETECT_INPUT & _BV(SD_DETECT_PIN)));
        printf("Res: %d\r\n", res);
        delay_ms(500);
    }

    char str[12];
    DWORD sn;
    f_getlabel("", str, &sn);

    printf("Label: %s, SN: %lu\r\n", str, sn);

    FIL fil;       
    char line[100];
    FRESULT fr;    

    
    fr = f_open(&fil, "file1.txt", FA_READ);
    printf("File open result: %d\r\n", fr);

    
    while (f_gets(line, sizeof line, &fil)) {
        printf(line);
    }

    f_close(&fil);

    printf("Returning from SD test\r\n");
}

// TODO: Get time from RTC or host
// DWORD get_fattime (void)
// {
//     // Returns current time packed into a DWORD variable 
//     return    ((DWORD)(2013 - 1980) << 25)  // Year 2013 
//     | ((DWORD)8 << 21)              // Month 7 
//     | ((DWORD)2 << 16)              // Mday 28 
//     | ((DWORD)20 << 11)             // Hour 0..24
//     | ((DWORD)30 << 5)              // Min 0 
//     | ((DWORD)0 >> 1);              // Sec 0
// }
#include "oled.h"

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <log/log.h>

#include "../core/common.hh"
#include "../core/defines.h"
#include "i2c.h"
#include "msp_displayport.h"
#include "uart.h"

extern int GOGGLE_VER_2;

#define _OLED_TEMP_TEST

// OLED access
/*
        V536  --m_i2c-->  AL FPGA  --I2C-->  OLED
                                  --RESX 5VEN -5VEN 1V8EN--> OLED

    SPI:
        page: 3bit, addr: 12bit, data: 32bit

    AL FPGA reg:
        reg_a0[1:0]: 1=write cmd; 2=read cmd; self-clear
        reg_a1: addr[7:0]
        reg_a2: addr[15:8]
        reg_a3: wr_data[7:0]
        reg_a4: wr_data[15:8]
        reg_a5: rd_data_right[7:0]   (read only)
        reg_a6: rd_data_right[15:8]  (read only)
        reg_a7: rd_data_left[7:0]    (read only)
        reg_a8: rd_data_left[15:8]   (read only)

*/

void OLED_write(uint16_t addr, uint16_t wdat, uint8_t sel) {
    uint8_t val;

    if (GOGGLE_VER_2 == 0) {
        val = addr & 0xFF;
        I2C_Write(ADDR_AL, 0xa1, val);
        val = (addr >> 8) & 0xFF;
        I2C_Write(ADDR_AL, 0xa2, val);

        val = wdat & 0xFF;
        I2C_Write(ADDR_AL, 0xa3, val);
        val = (wdat >> 8) & 0xFF;
        I2C_Write(ADDR_AL, 0xa4, val);

        val = (sel << 4) | 0x01;
        I2C_Write(ADDR_AL, 0xa0, val);
    } else {
        val = addr & 0xFF;
        I2C_Write(ADDR_FPGA, 0xa9, val);
        val = (addr >> 8) & 0xFF;
        I2C_Write(ADDR_FPGA, 0xaa, val);

        val = wdat & 0xFF;
        I2C_Write(ADDR_FPGA, 0xab, val);
        val = (wdat >> 8) & 0xFF;
        I2C_Write(ADDR_FPGA, 0xac, val);

        val = (sel << 4) | 0x01;
        I2C_Write(ADDR_FPGA, 0xa8, val);
    }

    usleep(250);
}

uint16_t OLED_read(uint16_t addr, uint8_t sel) {
    uint8_t val;
    uint16_t rdat;

    if (GOGGLE_VER_2 == 0) {
        val = addr & 0xFF;
        I2C_Write(ADDR_AL, 0xa1, val);
        val = (addr >> 8) & 0xFF;
        I2C_Write(ADDR_AL, 0xa2, val);

        val = (sel << 4) | 0x02;
        I2C_Write(ADDR_AL, 0xa0, val);

        usleep(500);

        val = I2C_Read(ADDR_AL, 0xa6);
        rdat = val;
        rdat <<= 8;
        val = I2C_Read(ADDR_AL, 0xa5);
        rdat |= val;
    } else {
        val = addr & 0xFF;
        I2C_Write(ADDR_FPGA, 0xa9, val);
        val = (addr >> 8) & 0xFF;
        I2C_Write(ADDR_FPGA, 0xaa, val);

        val = (sel << 4) | 0x02;
        I2C_Write(ADDR_FPGA, 0xa8, val);

        usleep(500);

        val = I2C_Read(ADDR_FPGA, 0xae);
        rdat = val;
        rdat <<= 8;
        val = I2C_Read(ADDR_FPGA, 0xad);
        rdat |= val;
    }

    usleep(250);
    return rdat;
}

void OLED_Startup() {
    uint16_t l0, l1, l2, l3, l4;
    uint16_t r0, r1, r2, r3, r4;

    if (GOGGLE_VER_2 == 0) {
        if (!(I2C_Read(ADDR_AL, 0x00) & 0x01)) {
            LOGW("OLED_Startup failed: Auto init is not ready...");
        }
    }

    if (GOGGLE_VER_2 == 0) {
        I2C_Write(ADDR_AL, 0x10, 0x01);
    } else {
        I2C_Write(ADDR_FPGA, 0xa0, 0x01);
    }

    usleep(1000);

#ifdef _OLED_TEMP_TEST
    OLED_write(0xF000, 0x00AA, 2);
    OLED_write(0xF001, 0x0012, 2);

    l0 = OLED_read(0xD000, 0);
    l1 = OLED_read(0xD001, 0);
    l2 = OLED_read(0xD002, 0);
    l3 = OLED_read(0xD003, 0);
    l4 = OLED_read(0xD004, 0);

    r0 = OLED_read(0xD000, 1);
    r1 = OLED_read(0xD001, 1);
    r2 = OLED_read(0xD002, 1);
    r3 = OLED_read(0xD003, 1);
    r4 = OLED_read(0xD004, 1);

    LOGI("OLED temp test: 0xD0 L = %x  %x  %x  %x  %x  ", l0, l1, l2, l3, l4);
    LOGI("OLED temp test: 0xD0 R = %x  %x  %x  %x  %x  ", r0, r1, r2, r3, r4);

    if (l0 == 0x05) {
        OLED_write(0xD000, 0x000A, 0);
        OLED_write(0xD001, 0x000A, 0);
        OLED_write(0xD002, l2 + 32, 0);
        OLED_write(0xD003, l3 + 32, 0);
        OLED_write(0xD004, l4 + 32, 0);
    }

    if (r0 == 0x05) {
        OLED_write(0xD000, 0x000A, 1);
        OLED_write(0xD001, 0x000A, 1);
        OLED_write(0xD002, r2 + 32, 1);
        OLED_write(0xD003, r3 + 32, 1);
        OLED_write(0xD004, r4 + 32, 1);
    }

    l0 = OLED_read(0xD000, 0);
    l1 = OLED_read(0xD001, 0);
    l2 = OLED_read(0xD002, 0);
    l3 = OLED_read(0xD003, 0);
    l4 = OLED_read(0xD004, 0);

    r0 = OLED_read(0xD000, 1);
    r1 = OLED_read(0xD001, 1);
    r2 = OLED_read(0xD002, 1);
    r3 = OLED_read(0xD003, 1);
    r4 = OLED_read(0xD004, 1);

    LOGI("OLED temp test modified: 0xD0 L = %x  %x  %x  %x  %x  ", l0, l1, l2, l3, l4);
    LOGI("OLED temp test modified: 0xD0 R = %x  %x  %x  %x  %x  ", r0, r1, r2, r3, r4);
#endif
}

// Set OLED to pattern mode
// enable: 0=disable; 1=enable
// mode: 0=color bar; 1=grid; 2=all black; 3=all white; 4=boot screen
// speed: color bar move speed (0~15)
void OLED_Pattern(uint8_t enable, uint8_t mode, uint8_t speed) {
    mode = (enable & 0x01) | ((mode & 0x07) << 1) | ((speed & 0x0F) << 4);

    OLED_display(0);
    if (GOGGLE_VER_2 == 0) {
        I2C_Write(ADDR_AL, 0x15, mode);
    } else {
        I2C_Write(ADDR_FPGA, 0xa4, mode);
    }
    OLED_display(1);
}

void OLED_SetTMG(int mode) // mode: 0=1080P; 1=720P
{
    static int last_mode = 0;

    if (last_mode != mode) {
        last_mode = mode;
        switch (mode) {
        case 0:
            I2C_Write(ADDR_AL, 0x33, 0x04);
            OLED_write(0x8001, 0x00E0, 2);
            OLED_write(0x6900, 0x0000, 2);
            break;
        case 1:
            I2C_Write(ADDR_AL, 0x33, 0x04);
            OLED_write(0x8001, 0x0040, 2);
            OLED_write(0x6900, 0x0002, 2);
            break;
        case 2:
            I2C_Write(ADDR_AL, 0x33, 0x04);
            I2C_Write(ADDR_AL, 0x16, 0x00);
            OLED_write(0x8001, 0x0068, 2);
            OLED_write(0x6900, 0x0001, 2);
            break;
        }
        LOGI("OLED: Set to mode %d.", mode);
    }
}

void MFPGA_Set720P90(uint8_t mode) {
    I2C_Write(ADDR_FPGA, 0x40, 0xc0);
    I2C_Write(ADDR_FPGA, 0x41, 0x23);
    I2C_Write(ADDR_FPGA, 0x42, 0x1c);
    I2C_Write(ADDR_FPGA, 0x43, 0xaa);
    I2C_Write(ADDR_FPGA, 0x44, 0x45);
    I2C_Write(ADDR_FPGA, 0x45, 0x39);
    I2C_Write(ADDR_FPGA, 0x46, 0x00);
    I2C_Write(ADDR_FPGA, 0x47, 0x00);
    I2C_Write(ADDR_FPGA, 0x48, 0x28);
    I2C_Write(ADDR_FPGA, 0x49, 0xdd);
    I2C_Write(ADDR_FPGA, 0x4a, 0x01);
    I2C_Write(ADDR_FPGA, 0x4b, 0x05);
    I2C_Write(ADDR_FPGA, 0x4c, 0x11);

    if (mode == VR_540P90_CROP) {
        I2C_Write(ADDR_FPGA, 0x4d, 0xE2);
        I2C_Write(ADDR_FPGA, 0x4e, 0x04);
    } else {
        I2C_Write(ADDR_FPGA, 0x4d, 0x30);
        I2C_Write(ADDR_FPGA, 0x4e, 0x05);
    }

    I2C_Write(ADDR_FPGA, 0x4f, 0x00);
    I2C_Write(ADDR_FPGA, 0x52, 0x48);
    I2C_Write(ADDR_FPGA, 0x53, 0x48);
    I2C_Write(ADDR_FPGA, 0x54, 0x66);
    I2C_Write(ADDR_FPGA, 0x61, 0x71);
    I2C_Write(ADDR_FPGA, 0x63, 0x5a);
    I2C_Write(ADDR_FPGA, 0x65, 0x96);
    I2C_Write(ADDR_FPGA, 0x66, 0x00);

    MFPGA_SetRatio(1);
    I2C_Write(ADDR_FPGA, 0x06, 0x0F);
}

void MFPGA_Set540P60() {
    I2C_Write(ADDR_FPGA, 0x40, 0xc0);
    I2C_Write(ADDR_FPGA, 0x41, 0x23);
    I2C_Write(ADDR_FPGA, 0x42, 0x1c);
    I2C_Write(ADDR_FPGA, 0x43, 0x4b);
    I2C_Write(ADDR_FPGA, 0x44, 0x44);
    I2C_Write(ADDR_FPGA, 0x45, 0x33);
    I2C_Write(ADDR_FPGA, 0x46, 0x00);
    I2C_Write(ADDR_FPGA, 0x47, 0x00);
    I2C_Write(ADDR_FPGA, 0x48, 0x28);
    I2C_Write(ADDR_FPGA, 0x49, 0x84);
    I2C_Write(ADDR_FPGA, 0x4a, 0x00);
    I2C_Write(ADDR_FPGA, 0x4b, 0x05);
    I2C_Write(ADDR_FPGA, 0x4c, 0x11);

    I2C_Write(ADDR_FPGA, 0x4d, 0xd0);
    I2C_Write(ADDR_FPGA, 0x4e, 0x07);

    I2C_Write(ADDR_FPGA, 0x4f, 0x00);
    I2C_Write(ADDR_FPGA, 0x52, 0x48);
    I2C_Write(ADDR_FPGA, 0x53, 0x48);
    I2C_Write(ADDR_FPGA, 0x54, 0x66);
    I2C_Write(ADDR_FPGA, 0x61, 0x71);
    I2C_Write(ADDR_FPGA, 0x63, 0x5a);
    I2C_Write(ADDR_FPGA, 0x65, 0x96);
    I2C_Write(ADDR_FPGA, 0x66, 0x00);

    MFPGA_SetRatio(1);
    I2C_Write(ADDR_FPGA, 0x06, 0x0F);
}

void MFPGA_Set720P60(uint8_t mode, uint8_t is_43) {
    I2C_Write(ADDR_FPGA, 0x40, 0x00);
    I2C_Write(ADDR_FPGA, 0x41, 0x25);
    I2C_Write(ADDR_FPGA, 0x42, 0xd0);
    I2C_Write(ADDR_FPGA, 0x43, 0x72);
    I2C_Write(ADDR_FPGA, 0x44, 0x46);
    I2C_Write(ADDR_FPGA, 0x45, 0xee);
    I2C_Write(ADDR_FPGA, 0x46, 0x00);
    I2C_Write(ADDR_FPGA, 0x47, 0x00);
    I2C_Write(ADDR_FPGA, 0x48, 0x28);
    I2C_Write(ADDR_FPGA, 0x49, 0xf7);
    I2C_Write(ADDR_FPGA, 0x4a, 0x00);
    I2C_Write(ADDR_FPGA, 0x4b, 0x05);
    I2C_Write(ADDR_FPGA, 0x4c, 0x19);

    if (mode == VR_960x720P60) {
        I2C_Write(ADDR_FPGA, 0x4d, 0xDC);
        I2C_Write(ADDR_FPGA, 0x4e, 0x05);
    } else if (mode == VR_720P50) {
        I2C_Write(ADDR_FPGA, 0x4d, 0xBC);
        I2C_Write(ADDR_FPGA, 0x4e, 0x07);
    } else {
        I2C_Write(ADDR_FPGA, 0x4d, 0x72);
        I2C_Write(ADDR_FPGA, 0x4e, 0x06);
    }

    I2C_Write(ADDR_FPGA, 0x4f, 0x00);
    I2C_Write(ADDR_FPGA, 0x52, 0x5f);
    I2C_Write(ADDR_FPGA, 0x53, 0x5f);
    I2C_Write(ADDR_FPGA, 0x54, 0x88);
    I2C_Write(ADDR_FPGA, 0x61, 0x96);
    I2C_Write(ADDR_FPGA, 0x63, 0x78);
    I2C_Write(ADDR_FPGA, 0x65, 0xc8);
    I2C_Write(ADDR_FPGA, 0x66, 0x00);

    MFPGA_SetRatio(is_43);
    I2C_Write(ADDR_FPGA, 0x06, 0x0F);
}

void MFPGA_Set1080P30() {
    I2C_Write(ADDR_FPGA, 0x40, 0x80);
    I2C_Write(ADDR_FPGA, 0x41, 0x47);
    I2C_Write(ADDR_FPGA, 0x42, 0x38);
    I2C_Write(ADDR_FPGA, 0x43, 0x9a);
    I2C_Write(ADDR_FPGA, 0x44, 0x88);
    I2C_Write(ADDR_FPGA, 0x45, 0x64);
    I2C_Write(ADDR_FPGA, 0x46, 0x00);
    I2C_Write(ADDR_FPGA, 0x47, 0x00);
    I2C_Write(ADDR_FPGA, 0x48, 0x2c);
    I2C_Write(ADDR_FPGA, 0x49, 0xa9);
    I2C_Write(ADDR_FPGA, 0x4a, 0x00);
    I2C_Write(ADDR_FPGA, 0x4b, 0x05);
    I2C_Write(ADDR_FPGA, 0x4c, 0x28);

    I2C_Write(ADDR_FPGA, 0x4d, 0x98);
    I2C_Write(ADDR_FPGA, 0x4e, 0x08);

    I2C_Write(ADDR_FPGA, 0x4f, 0x00);
    I2C_Write(ADDR_FPGA, 0x52, 0x8f);
    I2C_Write(ADDR_FPGA, 0x53, 0x8f);
    I2C_Write(ADDR_FPGA, 0x54, 0xcc);
    I2C_Write(ADDR_FPGA, 0x61, 0xe1);
    I2C_Write(ADDR_FPGA, 0x63, 0xb4);
    I2C_Write(ADDR_FPGA, 0x65, 0x2c);
    I2C_Write(ADDR_FPGA, 0x66, 0x01);

    MFPGA_SetRatio(0);
    I2C_Write(ADDR_FPGA, 0x06, 0x0F);
}

void MFPGA_SetRatio(int ratio) {
    if (ratio)
        I2C_Write(ADDR_FPGA, 0x8f, 0x80);
    else
        I2C_Write(ADDR_FPGA, 0x8f, 0x00);
    // LOGI("MFPGA_SetRatio %d",ratio);
}

void OLED_ReadBack();

// OLED display on/off
void OLED_display(int on) {
    static int last_on = -1;

    if (GOGGLE_VER_2) {
        if (last_on != on)
            last_on = on;
        else
            return;
    }

    if (on) {
        if (GOGGLE_VER_2 == 0)
            I2C_Write(ADDR_AL, 0x13, 0x83);
        else {
            // I2C_Write(ADDR_FPGA, 0xa3, 0x81);
            // usleep(50000);
            I2C_Write(ADDR_FPGA, 0xa3, 0x83);
        }

        usleep(1000);
        OLED_write(0x8000, 0x0001, 2);
        usleep(5000);
        OLED_write(0x2900, 0x0000, 2); // display on
        usleep(20000);
        OLED_write(0x5300, 0x0029, 2);
        OLED_write(0x5100, 0x00FF, 2);
        OLED_write(0x5101, 0x0001, 2);
        OLED_write(0x0300, 0x0000, 2);
        usleep(1000);

        if (GOGGLE_VER_2 == 0)
            I2C_Write(ADDR_AL, 0x13, 0x03);
        else {
            // usleep(50000);
            // I2C_Write(ADDR_FPGA, 0xa3, 0x83);
            // usleep(20000);
            I2C_Write(ADDR_FPGA, 0xa3, 0x03);
        }

        LOGI("OLED: Display on");
        // if (GOGGLE_VER_2) {
        // usleep(1000000);
        // OLED_ReadBack();
        //}

    } else {
        if (GOGGLE_VER_2 == 0)
            I2C_Write(ADDR_AL, 0x13, 0x83);
        else
            I2C_Write(ADDR_FPGA, 0xa3, 0x83);

        OLED_write(0x2800, 0x0000, 2); // display off
        usleep(20000);

        if (GOGGLE_VER_2 == 0)
            I2C_Write(ADDR_AL, 0x13, 0x80);
        else
            I2C_Write(ADDR_FPGA, 0xa3, 0x80);

        LOGI("OLED: Display off");
    }
}

// OLED power off
void OLED_power_down() {
    if (GOGGLE_VER_2 == 0) {
        I2C_Write(ADDR_AL, 0x13, 0x01);
        usleep(1000);
        OLED_write(0x2800, 0x0000, 2); // display off
        usleep(1000);
        OLED_write(0x1000, 0x0000, 2); // sleep-in
        usleep(1000);

        I2C_Write(ADDR_AL, 0x11, 0x00); // RESX
        usleep(1000);

        I2C_Write(ADDR_AL, 0x12, 0x03); // AVEE disable
        usleep(2000);
        I2C_Write(ADDR_AL, 0x12, 0x01); // AVDD disable
        usleep(1000);
        I2C_Write(ADDR_AL, 0x12, 0x00); // VDDI disable
    } else {
        I2C_Write(ADDR_FPGA, 0xa3, 0x80);
        usleep(1000);
        OLED_write(0x2800, 0x0000, 2); // display off
        usleep(1000);
        OLED_write(0x1000, 0x0000, 2); // sleep-in
        usleep(1000);

        I2C_Write(ADDR_FPGA, 0xa1, 0x00); // RESX
        usleep(1000);

        I2C_Write(ADDR_FPGA, 0xa2, 0x03); // AVEE disable
        usleep(2000);
        I2C_Write(ADDR_FPGA, 0xa2, 0x01); // AVDD disable
        usleep(1000);
        I2C_Write(ADDR_FPGA, 0xa2, 0x00); // VDDI disable

        usleep(100000);
    }
}

// OLED brightness setting
void OLED_Brightness(uint8_t level) {
    uint16_t dh = 0, dl = 0;

    switch (level) {
    case 12:
        dh = 0x0002;
        dl = 0x00F0;
        break;
    case 11:
        dh = 0x0002;
        dl = 0x00C0;
        break;
    case 10:
        dh = 0x0002;
        dl = 0x0090;
        break;
    case 9:
        dh = 0x0002;
        dl = 0x0060;
        break;
    case 8:
        dh = 0x0002;
        dl = 0x0030;
        break;
    case 7:
        dh = 0x0002;
        dl = 0x0000;
        break;
    case 6:
        dh = 0x0001;
        dl = 0x00D0;
        break;
    case 5:
        dh = 0x0001;
        dl = 0x00A0;
        break;
    case 4:
        dh = 0x0001;
        dl = 0x0070;
        break;
    case 3:
        dh = 0x0001;
        dl = 0x0040;
        break;
    case 2:
        dh = 0x0001;
        dl = 0x0010;
        break;
    case 1:
        dh = 0x0000;
        dl = 0x00E0;
        break;
    case 0:
        dh = 0x0000;
        dl = 0x0020;
        break;
    }

    OLED_write(0xF000, 0x00AA, 2);
    OLED_write(0xF001, 0x0011, 2);

    OLED_write(0xC200, dh, 2);
    OLED_write(0xC201, dl, 2);
    OLED_write(0xC202, dh, 2);
    OLED_write(0xC203, dl, 2);
    OLED_write(0xC204, dh, 2);
    OLED_write(0xC205, dl, 2);
    OLED_write(0xC206, dh, 2);
    OLED_write(0xC207, dl, 2);
}

// only support goggle 1v1
void OLED_ReadBack() {
    uint16_t l0, l1, l2, l3, l4;
    uint16_t r0, r1, r2, r3, r4;

    if (GOGGLE_VER_2 == 0)
        return;

    LOGI("\n ");

    OLED_write(0xF000, 0x00AA, 2);
    OLED_write(0xF001, 0x0012, 2);

    l0 = OLED_read(0x5300, 0);
    r0 = OLED_read(0x5300, 1);
    LOGI("OLED_ReadBack: 0x5300 = %x  %x", l0, r0);

    l0 = OLED_read(0x5100, 0);
    r0 = OLED_read(0x5100, 1);
    LOGI("OLED_ReadBack: 0x5100 = %x  %x", l0, r0);

    l0 = OLED_read(0x5101, 0);
    r0 = OLED_read(0x5101, 1);
    LOGI("OLED_ReadBack: 0x5101 = %x  %x", l0, r0);

    l0 = OLED_read(0x0300, 0);
    r0 = OLED_read(0x0300, 1);
    LOGI("OLED_ReadBack: 0x0300 = %x  %x", l0, r0);

    l0 = OLED_read(0x8000, 0);
    r0 = OLED_read(0x8000, 1);
    LOGI("OLED_ReadBack: 0x8000 = %x  %x", l0, r0);

    l0 = OLED_read(0x8001, 0);
    r0 = OLED_read(0x8001, 1);
    LOGI("OLED_ReadBack: 0x8001 = %x  %x", l0, r0);

    l0 = OLED_read(0x8002, 0);
    r0 = OLED_read(0x8002, 1);
    LOGI("OLED_ReadBack: 0x8002 = %x  %x", l0, r0);

    l0 = OLED_read(0x8003, 0);
    r0 = OLED_read(0x8003, 1);
    LOGI("OLED_ReadBack: 0x8003 = %x  %x", l0, r0);

    l0 = OLED_read(0x8004, 0);
    r0 = OLED_read(0x8004, 1);
    LOGI("OLED_ReadBack: 0x8004 = %x  %x", l0, r0);

    l0 = OLED_read(0x8005, 0);
    r0 = OLED_read(0x8005, 1);
    LOGI("OLED_ReadBack: 0x8005 = %x  %x", l0, r0);

    l0 = OLED_read(0x8100, 0);
    r0 = OLED_read(0x8100, 1);
    LOGI("OLED_ReadBack: 0x8100 = %x  %x", l0, r0);

    l0 = OLED_read(0x8101, 0);
    r0 = OLED_read(0x8101, 1);
    LOGI("OLED_ReadBack: 0x8101 = %x  %x", l0, r0);

    l0 = OLED_read(0x8102, 0);
    r0 = OLED_read(0x8102, 1);
    LOGI("OLED_ReadBack: 0x8102 = %x  %x", l0, r0);

    l0 = OLED_read(0x8103, 0);
    r0 = OLED_read(0x8103, 1);
    LOGI("OLED_ReadBack: 0x8103 = %x  %x", l0, r0);

    l0 = OLED_read(0x8104, 0);
    r0 = OLED_read(0x8104, 1);
    LOGI("OLED_ReadBack: 0x8104 = %x  %x", l0, r0);

    l0 = OLED_read(0x8105, 0);
    r0 = OLED_read(0x8105, 1);
    LOGI("OLED_ReadBack: 0x8105 = %x  %x", l0, r0);

    l0 = OLED_read(0x8106, 0);
    r0 = OLED_read(0x8106, 1);
    LOGI("OLED_ReadBack: 0x8106 = %x  %x", l0, r0);

    l0 = OLED_read(0x8200, 0);
    r0 = OLED_read(0x8200, 1);
    LOGI("OLED_ReadBack: 0x8200 = %x  %x", l0, r0);

    l0 = OLED_read(0x8201, 0);
    r0 = OLED_read(0x8201, 1);
    LOGI("OLED_ReadBack: 0x8201 = %x  %x", l0, r0);

    l0 = OLED_read(0x8202, 0);
    r0 = OLED_read(0x8202, 1);
    LOGI("OLED_ReadBack: 0x8202 = %x  %x", l0, r0);

    l0 = OLED_read(0x8203, 0);
    r0 = OLED_read(0x8203, 1);
    LOGI("OLED_ReadBack: 0x8203 = %x  %x", l0, r0);

    l0 = OLED_read(0x8204, 0);
    r0 = OLED_read(0x8204, 1);
    LOGI("OLED_ReadBack: 0x8204 = %x  %x", l0, r0);

    l0 = OLED_read(0x8205, 0);
    r0 = OLED_read(0x8205, 1);
    LOGI("OLED_ReadBack: 0x8205 = %x  %x", l0, r0);

    l0 = OLED_read(0x8206, 0);
    r0 = OLED_read(0x8206, 1);
    LOGI("OLED_ReadBack: 0x8206 = %x  %x", l0, r0);

    l0 = OLED_read(0x3500, 0);
    r0 = OLED_read(0x3500, 1);
    LOGI("OLED_ReadBack: 0x3500 = %x  %x", l0, r0);

    l0 = OLED_read(0x2600, 0);
    r0 = OLED_read(0x2600, 1);
    LOGI("OLED_ReadBack: 0x2600 = %x  %x", l0, r0);

    l0 = OLED_read(0x6900, 0);
    r0 = OLED_read(0x6900, 1);
    LOGI("OLED_ReadBack: 0x6900 = %x  %x", l0, r0);

    OLED_write(0xF000, 0x00AA, 2);
    OLED_write(0xF001, 0x0011, 2);

    l0 = OLED_read(0xC200, 0);
    r0 = OLED_read(0xC200, 1);
    LOGI("OLED_ReadBack: 0xC200 = %x  %x", l0, r0);

    l0 = OLED_read(0xC201, 0);
    r0 = OLED_read(0xC201, 1);
    LOGI("OLED_ReadBack: 0xC201 = %x  %x", l0, r0);

    l0 = OLED_read(0xC202, 0);
    r0 = OLED_read(0xC202, 1);
    LOGI("OLED_ReadBack: 0xC202 = %x  %x", l0, r0);

    l0 = OLED_read(0xC203, 0);
    r0 = OLED_read(0xC203, 1);
    LOGI("OLED_ReadBack: 0xC203 = %x  %x", l0, r0);

    l0 = OLED_read(0xC204, 0);
    r0 = OLED_read(0xC204, 1);
    LOGI("OLED_ReadBack: 0xC204 = %x  %x", l0, r0);

    l0 = OLED_read(0xC205, 0);
    r0 = OLED_read(0xC205, 1);
    LOGI("OLED_ReadBack: 0xC205 = %x  %x", l0, r0);

    l0 = OLED_read(0xC206, 0);
    r0 = OLED_read(0xC206, 1);
    LOGI("OLED_ReadBack: 0xC206 = %x  %x", l0, r0);

    l0 = OLED_read(0xC207, 0);
    r0 = OLED_read(0xC207, 1);
    LOGI("OLED_ReadBack: 0xC207 = %x  %x", l0, r0);

    OLED_write(0xF000, 0x00AA, 2);
    OLED_write(0xF001, 0x0012, 2);

    l0 = OLED_read(0xD000, 0);
    r0 = OLED_read(0xD000, 1);
    LOGI("OLED_ReadBack: 0xD000 = %x  %x", l0, r0);

    l0 = OLED_read(0xD001, 0);
    r0 = OLED_read(0xD001, 1);
    LOGI("OLED_ReadBack: 0xD001 = %x  %x", l0, r0);

    l0 = OLED_read(0xD002, 0);
    r0 = OLED_read(0xD002, 1);
    LOGI("OLED_ReadBack: 0xD002 = %x  %x", l0, r0);

    l0 = OLED_read(0xD003, 0);
    r0 = OLED_read(0xD003, 1);
    LOGI("OLED_ReadBack: 0xD003 = %x  %x", l0, r0);

    l0 = OLED_read(0xD004, 0);
    r0 = OLED_read(0xD004, 1);
    LOGI("OLED_ReadBack: 0xD004 = %x  %x", l0, r0);

    l0 = OLED_read(0xBF00, 0);
    r0 = OLED_read(0xBF00, 1);
    LOGI("OLED_ReadBack: 0xBF00 = %x  %x", l0, r0);

    l0 = OLED_read(0xBF01, 0);
    r0 = OLED_read(0xBF01, 1);
    LOGI("OLED_ReadBack: 0xBF01 = %x  %x", l0, r0);

    OLED_write(0xFF00, 0x005A, 2);
    OLED_write(0xFF01, 0x0080, 2);

    l0 = OLED_read(0xF22F, 0);
    r0 = OLED_read(0xF22F, 1);
    LOGI("OLED_ReadBack: 0xF22F = %x  %x", l0, r0);

    OLED_write(0xFF00, 0x005A, 2);
    OLED_write(0xFF01, 0x0081, 2);

    l0 = OLED_read(0xF205, 0);
    r0 = OLED_read(0xF205, 1);
    LOGI("OLED_ReadBack: 0xF205 = %x  %x", l0, r0);

    l0 = OLED_read(0xF20A, 0);
    r0 = OLED_read(0xF20A, 1);
    LOGI("OLED_ReadBack: 0xF20A = %x  %x", l0, r0);

    l0 = OLED_read(0xF917, 0);
    r0 = OLED_read(0xF917, 1);
    LOGI("OLED_ReadBack: 0xF917 = %x  %x", l0, r0);

    l0 = OLED_read(0xF918, 0);
    r0 = OLED_read(0xF918, 1);
    LOGI("OLED_ReadBack: 0xF918 = %x  %x", l0, r0);

    l0 = OLED_read(0xF919, 0);
    r0 = OLED_read(0xF919, 1);
    LOGI("OLED_ReadBack: 0xF919 = %x  %x", l0, r0);

    l0 = OLED_read(0xF91A, 0);
    r0 = OLED_read(0xF91A, 1);
    LOGI("OLED_ReadBack: 0xF91A = %x  %x", l0, r0);

    l0 = OLED_read(0xF91B, 0);
    r0 = OLED_read(0xF91B, 1);
    LOGI("OLED_ReadBack: 0xF91B = %x  %x", l0, r0);

    l0 = OLED_read(0xF91C, 0);
    r0 = OLED_read(0xF91C, 1);
    LOGI("OLED_ReadBack: 0xF91C = %x  %x", l0, r0);

    l0 = OLED_read(0xF91D, 0);
    r0 = OLED_read(0xF91D, 1);
    LOGI("OLED_ReadBack: 0xF91D = %x  %x", l0, r0);

    l0 = OLED_read(0xF91E, 0);
    r0 = OLED_read(0xF91E, 1);
    LOGI("OLED_ReadBack: 0xF91E = %x  %x", l0, r0);

    l0 = OLED_read(0xF91F, 0);
    r0 = OLED_read(0xF91F, 1);
    LOGI("OLED_ReadBack: 0xF91F = %x  %x", l0, r0);

    l0 = OLED_read(0xF920, 0);
    r0 = OLED_read(0xF920, 1);
    LOGI("OLED_ReadBack: 0xF920 = %x  %x", l0, r0);

    l0 = OLED_read(0xF921, 0);
    r0 = OLED_read(0xF921, 1);
    LOGI("OLED_ReadBack: 0xF921 = %x  %x", l0, r0);

    l0 = OLED_read(0xF922, 0);
    r0 = OLED_read(0xF922, 1);
    LOGI("OLED_ReadBack: 0xF922 = %x  %x", l0, r0);

    l0 = OLED_read(0xF923, 0);
    r0 = OLED_read(0xF923, 1);
    LOGI("OLED_ReadBack: 0xF923 = %x  %x", l0, r0);

    OLED_write(0xF000, 0x00AA, 2);
    OLED_write(0xF001, 0x0011, 2);
    OLED_write(0xC000, 0x0000, 2);

    r0 = OLED_read(0x3600, 1);
    LOGI("OLED_ReadBack only R: 0x3600 = %x", r0);

    OLED_write(0xF000, 0x00AA, 1);
    OLED_write(0xF001, 0x0013, 1);

    r0 = OLED_read(0xC101, 1);
    LOGI("OLED_ReadBack only R: 0xC101 = %x", r0);

    r0 = OLED_read(0xC406, 1);
    LOGI("OLED_ReadBack only R: 0xC406 = %x", r0);

    r0 = OLED_read(0xC407, 1);
    LOGI("OLED_ReadBack only R: 0xC407 = %x", r0);

    r0 = OLED_read(0xC408, 1);
    LOGI("OLED_ReadBack only R: 0xC408 = %x", r0);

    r0 = OLED_read(0xC409, 1);
    LOGI("OLED_ReadBack only R: 0xC409 = %x", r0);

    r0 = OLED_read(0xC40A, 1);
    LOGI("OLED_ReadBack only R: 0xC40A = %x", r0);

    r0 = OLED_read(0xC40B, 1);
    LOGI("OLED_ReadBack only R: 0xC40B = %x", r0);

    OLED_write(0xF000, 0x00AA, 1);
    OLED_write(0xF001, 0x0016, 1);

    r0 = OLED_read(0xB606, 1);
    LOGI("OLED_ReadBack only R: 0xB606 = %x", r0);

    r0 = OLED_read(0xB607, 1);
    LOGI("OLED_ReadBack only R: 0xB607 = %x", r0);

    r0 = OLED_read(0xB608, 1);
    LOGI("OLED_ReadBack only R: 0xB608 = %x", r0);

    r0 = OLED_read(0xB609, 1);
    LOGI("OLED_ReadBack only R: 0xB609 = %x", r0);

    r0 = OLED_read(0xB60A, 1);
    LOGI("OLED_ReadBack only R: 0xB60A = %x", r0);

    r0 = OLED_read(0xB60B, 1);
    LOGI("OLED_ReadBack only R: 0xB60B = %x", r0);

    r0 = OLED_read(0xB000, 1);
    LOGI("OLED_ReadBack only R: 0xB000 = %x", r0);

    r0 = OLED_read(0xB001, 1);
    LOGI("OLED_ReadBack only R: 0xB001 = %x", r0);

    OLED_write(0xF000, 0x00AA, 2);
    OLED_write(0xF001, 0x0011, 2);
    OLED_write(0xC000, 0x00FF, 2);

    LOGI("\n ");
}

// only support goggle 1v1
void OLED_power_up() {
    if (GOGGLE_VER_2 == 0)
        return;
    I2C_Write(ADDR_FPGA, 0xa2, 0x01); // VDDI en
    usleep(2000);
    I2C_Write(ADDR_FPGA, 0xa2, 0x03); // AVDD en
    usleep(3000);
    I2C_Write(ADDR_FPGA, 0xa2, 0x07); // AVEE en

    usleep(1000);
    I2C_Write(ADDR_FPGA, 0xa3, 0x81); // open MIPI IF

    usleep(8000);
    I2C_Write(ADDR_FPGA, 0xa1, 0x01); // RESX
    usleep(22000);
}

// only support goggle 1v1
void OLED_reopen(int mode) { // mode: 0=1080P; 1=720P
    uint16_t l0, l1, l2, l3, l4;
    uint16_t r0, r1, r2, r3, r4;

    if (GOGGLE_VER_2 == 0)
        return;

    OLED_power_down();
    OLED_power_up();

    // #ifdef _OLED_TEMP_TEST
    OLED_write(0xF000, 0x00AA, 2);
    OLED_write(0xF001, 0x0012, 2);

    l0 = OLED_read(0xD000, 0);
    l1 = OLED_read(0xD001, 0);
    l2 = OLED_read(0xD002, 0);
    l3 = OLED_read(0xD003, 0);
    l4 = OLED_read(0xD004, 0);

    r0 = OLED_read(0xD000, 1);
    r1 = OLED_read(0xD001, 1);
    r2 = OLED_read(0xD002, 1);
    r3 = OLED_read(0xD003, 1);
    r4 = OLED_read(0xD004, 1);

    LOGI("OLED temp test: 0xD0 L = %x  %x  %x  %x  %x  ", l0, l1, l2, l3, l4);
    LOGI("OLED temp test: 0xD0 R = %x  %x  %x  %x  %x  ", r0, r1, r2, r3, r4);
    // #endif

    OLED_write(0x5300, 0x0029, 2);
    OLED_write(0x5100, 0x00FF, 2);
    OLED_write(0x5101, 0x0001, 2);
    OLED_write(0x0300, 0x0000, 2);
    OLED_write(0x8000, 0x0001, 2);
    OLED_write(0x8001, mode ? 0x0040 : 0x00E0, 2);
    OLED_write(0x8002, 0x00E0, 2);
    OLED_write(0x8003, 0x000E, 2);
    OLED_write(0x8004, 0x0000, 2);
    OLED_write(0x8005, 0x0031, 2);
    OLED_write(0x8100, 0x0003, 2);
    OLED_write(0x8101, 0x0004, 2);
    OLED_write(0x8102, 0x0000, 2);
    OLED_write(0x8103, 0x0029, 2);
    OLED_write(0x8104, 0x0000, 2);
    OLED_write(0x8105, 0x0004, 2);
    OLED_write(0x8106, 0x0000, 2);
    OLED_write(0x8200, 0x0003, 2);
    OLED_write(0x8201, 0x0004, 2);
    OLED_write(0x8202, 0x0000, 2);
    OLED_write(0x8203, 0x0029, 2);
    OLED_write(0x8204, 0x0000, 2);
    OLED_write(0x8205, 0x0004, 2);
    OLED_write(0x8206, 0x0001, 2);
    OLED_write(0x3500, 0x0000, 2);
    OLED_write(0x2600, 0x0020, 2);
    OLED_write(0x6900, mode ? 0x0002 : 0x0000, 2);

    OLED_write(0xF000, 0x00AA, 2);
    OLED_write(0xF001, 0x0011, 2);
    OLED_write(0xC200, 0x0001, 2);
    OLED_write(0xC201, 0x0090, 2);
    OLED_write(0xC202, 0x0001, 2);
    OLED_write(0xC203, 0x0090, 2);
    OLED_write(0xC204, 0x0001, 2);
    OLED_write(0xC205, 0x0090, 2);
    OLED_write(0xC206, 0x0001, 2);
    OLED_write(0xC207, 0x0090, 2);

    OLED_write(0xF000, 0x00AA, 2);
    OLED_write(0xF001, 0x0012, 2);
    // #ifdef _OLED_TEMP_TEST
    if (l0 == 0x05) {
        OLED_write(0xD000, 0x000A, 0);
        OLED_write(0xD001, 0x000A, 0);
        OLED_write(0xD002, l2 + 32, 0);
        OLED_write(0xD003, l3 + 32, 0);
        OLED_write(0xD004, l4 + 32, 0);
    }

    if (r0 == 0x05) {
        OLED_write(0xD000, 0x000A, 1);
        OLED_write(0xD001, 0x000A, 1);
        OLED_write(0xD002, r2 + 32, 1);
        OLED_write(0xD003, r3 + 32, 1);
        OLED_write(0xD004, r4 + 32, 1);
    }
    // #endif
    OLED_write(0xBF00, 0x0037, 2);
    OLED_write(0xBF01, 0x00A9, 2);

    OLED_write(0xFF00, 0x005A, 2);
    OLED_write(0xFF01, 0x0080, 2);
    OLED_write(0xF22F, 0x0001, 2);
    OLED_write(0xFF00, 0x005A, 2);
    OLED_write(0xFF01, 0x0081, 2);
    OLED_write(0xF205, 0x0022, 2);
    OLED_write(0xF20A, 0x0000, 2);
    OLED_write(0xF917, 0x005E, 2);
    OLED_write(0xF918, 0x0062, 2);
    OLED_write(0xF919, 0x0066, 2);
    OLED_write(0xF91A, 0x006A, 2);
    OLED_write(0xF91B, 0x006F, 2);
    OLED_write(0xF91C, 0x0073, 2);
    OLED_write(0xF91D, 0x0077, 2);
    OLED_write(0xF91E, 0x007B, 2);
    OLED_write(0xF91F, 0x007F, 2);
    OLED_write(0xF920, 0x0084, 2);
    OLED_write(0xF921, 0x0088, 2);
    OLED_write(0xF922, 0x008C, 2);
    OLED_write(0xF923, 0x0090, 2);
    OLED_write(0xF000, 0x00AA, 2);
    OLED_write(0xF001, 0x0011, 2);
    OLED_write(0xC000, 0x0000, 2);

    OLED_write(0x3600, 0x0003, 1);
    OLED_write(0xF000, 0x00AA, 1);
    OLED_write(0xF001, 0x0013, 1);
    OLED_write(0xC101, 0x0022, 1);
    OLED_write(0xC406, 0x0031, 1);
    OLED_write(0xC407, 0x0042, 1);
    OLED_write(0xC408, 0x0056, 1);
    OLED_write(0xC409, 0x0012, 1);
    OLED_write(0xC40A, 0x0053, 1);
    OLED_write(0xC40B, 0x0064, 1);
    OLED_write(0xF000, 0x00AA, 1);
    OLED_write(0xF001, 0x0016, 1);
    OLED_write(0xB606, 0x0031, 1);
    OLED_write(0xB607, 0x0042, 1);
    OLED_write(0xB608, 0x0056, 1);
    OLED_write(0xB609, 0x0012, 1);
    OLED_write(0xB60A, 0x0053, 1);
    OLED_write(0xB60B, 0x0064, 1);
    OLED_write(0xB000, 0x0000, 1);
    OLED_write(0xB001, 0x0044, 1);

    usleep(20000);
    OLED_write(0x1100, 0x0000, 2); // sleep-out
    usleep(100000);
    OLED_write(0x2900, 0x0000, 2); // display on
    usleep(20000);

    OLED_write(0xF000, 0x00AA, 2);
    OLED_write(0xF001, 0x0011, 2);
    OLED_write(0xC000, 0x00FF, 2);

    I2C_Write(ADDR_FPGA, 0xa3, 0x83); // mipi HS mode enable
    usleep(20000);
    I2C_Write(ADDR_FPGA, 0xa3, 0x03); // mipi HS mode enable
}

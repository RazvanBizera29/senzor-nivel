/*
 * ssd1306.c — Driver OLED SSD1306 128x64 I2C, bare metal
 *
 * Fara framebuffer (economie 1KB RAM pe ATmega328P).
 * Scriere directa in page mode.
 *
 * Font 5x7 stocat in PROGMEM (flash).
 * Fiecare caracter = 5 bytes (coloane), bit0=sus, bit6=jos.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "ssd1306.h"
#include "i2c.h"
#include "config.h"

/* =====================================================================
 * Font 5x7 — ASCII 0x20 (space) la 0x7E (~), 95 caractere × 5 bytes
 * Sursa: font clasic AVR/Arduino, verificat.
 * ===================================================================== */
static const uint8_t font5x7[][5] PROGMEM = {
    {0x00,0x00,0x00,0x00,0x00}, /* 0x20 space  */
    {0x00,0x00,0x5F,0x00,0x00}, /* 0x21 !      */
    {0x00,0x07,0x00,0x07,0x00}, /* 0x22 "      */
    {0x14,0x7F,0x14,0x7F,0x14}, /* 0x23 #      */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* 0x24 $      */
    {0x23,0x13,0x08,0x64,0x62}, /* 0x25 %      */
    {0x36,0x49,0x55,0x22,0x50}, /* 0x26 &      */
    {0x00,0x05,0x03,0x00,0x00}, /* 0x27 '      */
    {0x00,0x1C,0x22,0x41,0x00}, /* 0x28 (      */
    {0x00,0x41,0x22,0x1C,0x00}, /* 0x29 )      */
    {0x08,0x2A,0x1C,0x2A,0x08}, /* 0x2A *      */
    {0x08,0x08,0x3E,0x08,0x08}, /* 0x2B +      */
    {0x00,0x50,0x30,0x00,0x00}, /* 0x2C ,      */
    {0x08,0x08,0x08,0x08,0x08}, /* 0x2D -      */
    {0x00,0x60,0x60,0x00,0x00}, /* 0x2E .      */
    {0x20,0x10,0x08,0x04,0x02}, /* 0x2F /      */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0x30 0      */
    {0x00,0x42,0x7F,0x40,0x00}, /* 0x31 1      */
    {0x42,0x61,0x51,0x49,0x46}, /* 0x32 2      */
    {0x21,0x41,0x45,0x4B,0x31}, /* 0x33 3      */
    {0x18,0x14,0x12,0x7F,0x10}, /* 0x34 4      */
    {0x27,0x45,0x45,0x45,0x39}, /* 0x35 5      */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 0x36 6      */
    {0x01,0x71,0x09,0x05,0x03}, /* 0x37 7      */
    {0x36,0x49,0x49,0x49,0x36}, /* 0x38 8      */
    {0x06,0x49,0x49,0x29,0x1E}, /* 0x39 9      */
    {0x00,0x36,0x36,0x00,0x00}, /* 0x3A :      */
    {0x00,0x56,0x36,0x00,0x00}, /* 0x3B ;      */
    {0x00,0x08,0x14,0x22,0x41}, /* 0x3C <      */
    {0x14,0x14,0x14,0x14,0x14}, /* 0x3D =      */
    {0x41,0x22,0x14,0x08,0x00}, /* 0x3E >      */
    {0x02,0x01,0x51,0x09,0x06}, /* 0x3F ?      */
    {0x32,0x49,0x79,0x41,0x3E}, /* 0x40 @      */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 0x41 A      */
    {0x7F,0x49,0x49,0x49,0x36}, /* 0x42 B      */
    {0x3E,0x41,0x41,0x41,0x22}, /* 0x43 C      */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 0x44 D      */
    {0x7F,0x49,0x49,0x49,0x41}, /* 0x45 E      */
    {0x7F,0x09,0x09,0x09,0x01}, /* 0x46 F      */
    {0x3E,0x41,0x41,0x51,0x32}, /* 0x47 G      */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 0x48 H      */
    {0x00,0x41,0x7F,0x41,0x00}, /* 0x49 I      */
    {0x20,0x40,0x41,0x3F,0x01}, /* 0x4A J      */
    {0x7F,0x08,0x14,0x22,0x41}, /* 0x4B K      */
    {0x7F,0x40,0x40,0x40,0x40}, /* 0x4C L      */
    {0x7F,0x02,0x04,0x02,0x7F}, /* 0x4D M      */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 0x4E N      */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 0x4F O      */
    {0x7F,0x09,0x09,0x09,0x06}, /* 0x50 P      */
    {0x3E,0x41,0x51,0x21,0x5E}, /* 0x51 Q      */
    {0x7F,0x09,0x19,0x29,0x46}, /* 0x52 R      */
    {0x46,0x49,0x49,0x49,0x31}, /* 0x53 S      */
    {0x01,0x01,0x7F,0x01,0x01}, /* 0x54 T      */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 0x55 U      */
    {0x1F,0x20,0x40,0x20,0x1F}, /* 0x56 V      */
    {0x3F,0x40,0x38,0x40,0x3F}, /* 0x57 W      */
    {0x63,0x14,0x08,0x14,0x63}, /* 0x58 X      */
    {0x03,0x04,0x78,0x04,0x03}, /* 0x59 Y      */
    {0x61,0x51,0x49,0x45,0x43}, /* 0x5A Z      */
    {0x00,0x00,0x7F,0x41,0x41}, /* 0x5B [      */
    {0x02,0x04,0x08,0x10,0x20}, /* 0x5C \      */
    {0x41,0x41,0x7F,0x00,0x00}, /* 0x5D ]      */
    {0x04,0x02,0x01,0x02,0x04}, /* 0x5E ^      */
    {0x40,0x40,0x40,0x40,0x40}, /* 0x5F _      */
    {0x00,0x01,0x02,0x04,0x00}, /* 0x60 `      */
    {0x20,0x54,0x54,0x54,0x78}, /* 0x61 a      */
    {0x7F,0x48,0x44,0x44,0x38}, /* 0x62 b      */
    {0x38,0x44,0x44,0x44,0x20}, /* 0x63 c      */
    {0x38,0x44,0x44,0x48,0x7F}, /* 0x64 d      */
    {0x38,0x54,0x54,0x54,0x18}, /* 0x65 e      */
    {0x08,0x7E,0x09,0x01,0x02}, /* 0x66 f      */
    {0x08,0x54,0x54,0x54,0x3C}, /* 0x67 g      */
    {0x7F,0x08,0x04,0x04,0x78}, /* 0x68 h      */
    {0x00,0x44,0x7D,0x40,0x00}, /* 0x69 i      */
    {0x20,0x40,0x44,0x3D,0x00}, /* 0x6A j      */
    {0x7F,0x10,0x28,0x44,0x00}, /* 0x6B k      */
    {0x00,0x41,0x7F,0x40,0x00}, /* 0x6C l      */
    {0x7C,0x04,0x18,0x04,0x78}, /* 0x6D m      */
    {0x7C,0x08,0x04,0x04,0x78}, /* 0x6E n      */
    {0x38,0x44,0x44,0x44,0x38}, /* 0x6F o      */
    {0x7C,0x14,0x14,0x14,0x08}, /* 0x70 p      */
    {0x08,0x14,0x14,0x18,0x7C}, /* 0x71 q      */
    {0x7C,0x08,0x04,0x04,0x08}, /* 0x72 r      */
    {0x48,0x54,0x54,0x54,0x20}, /* 0x73 s      */
    {0x04,0x3F,0x44,0x40,0x20}, /* 0x74 t      */
    {0x3C,0x40,0x40,0x20,0x7C}, /* 0x75 u      */
    {0x1C,0x20,0x40,0x20,0x1C}, /* 0x76 v      */
    {0x3C,0x40,0x30,0x40,0x3C}, /* 0x77 w      */
    {0x44,0x28,0x10,0x28,0x44}, /* 0x78 x      */
    {0x0C,0x50,0x50,0x50,0x3C}, /* 0x79 y      */
    {0x44,0x64,0x54,0x4C,0x44}, /* 0x7A z      */
    {0x00,0x08,0x36,0x41,0x00}, /* 0x7B {      */
    {0x00,0x00,0x7F,0x00,0x00}, /* 0x7C |      */
    {0x00,0x41,0x36,0x08,0x00}, /* 0x7D }      */
    {0x08,0x08,0x2A,0x1C,0x08}, /* 0x7E ~      */
};

/* =====================================================================
 * Internale SSD1306
 * ===================================================================== */

static uint8_t cur_page = 0;
static uint8_t cur_col  = 0;

/* Trimite un singur byte de comanda */
static void oled_cmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};   /* 0x00 = Co=0, D/C#=0 (comanda) */
    i2c_write_buf(OLED_I2C_ADDR, buf, 2);
}

/* Trimite un singur byte de date (pixel) */
static void oled_data(uint8_t data)
{
    uint8_t buf[2] = {0x40, data};  /* 0x40 = Co=0, D/C#=1 (date) */
    i2c_write_buf(OLED_I2C_ADDR, buf, 2);
}

/* Trimite N bytes de date intr-o singura tranzactie I2C (eficient) */
static void oled_data_buf(const uint8_t *buf, uint8_t len)
{
    i2c_start((OLED_I2C_ADDR << 1) | 0);
    i2c_write(0x40);    /* control byte: date, continuare */
    for (uint8_t i = 0; i < len; i++) {
        i2c_write(buf[i]);
    }
    i2c_stop();
}

/* Seteaza pagina si coloana de start (page addressing mode) */
static void oled_set_pos(uint8_t page, uint8_t col)
{
    oled_cmd(0xB0 | (page & 0x07));         /* set page start address */
    oled_cmd(0x00 | (col & 0x0F));          /* set lower col nibble   */
    oled_cmd(0x10 | ((col >> 4) & 0x0F));   /* set upper col nibble   */
    cur_page = page;
    cur_col  = col;
}

/* =====================================================================
 * Initializare SSD1306
 * ===================================================================== */
void ssd1306_init(void)
{
    i2c_init();

    static const uint8_t init_seq[] = {
        0xAE,               /* display OFF */
        0xD5, 0x80,         /* clock divide ratio / oscillator freq */
        0xA8, 0x3F,         /* multiplex ratio: 64 rows (0x3F = 63) */
        0xD3, 0x00,         /* display offset: 0 */
        0x40,               /* display start line: 0 */
        0x8D, 0x14,         /* charge pump: enable */
        0x20, 0x02,         /* memory addressing: page mode */
        0xA1,               /* segment re-map: col 127 = SEG0 */
        0xC8,               /* COM scan direction: remapped */
        0xDA, 0x12,         /* COM pins hardware config */
        0x81, 0xCF,         /* contrast: 0xCF */
        0xD9, 0xF1,         /* pre-charge period */
        0xDB, 0x40,         /* VCOMH deselect level */
        0xA4,               /* entire display ON (from RAM) */
        0xA6,               /* normal display (not inverted) */
        0xAF,               /* display ON */
    };

    for (uint8_t i = 0; i < sizeof(init_seq); i++) {
        oled_cmd(init_seq[i]);
    }

    ssd1306_clear();
}

/* =====================================================================
 * Clear
 * ===================================================================== */
void ssd1306_clear(void)
{
    static uint8_t zeros[16];   /* scrie 16 bytes de 0 per tranzactie */
    for (uint8_t p = 0; p < 8; p++) {
        oled_set_pos(p, 0);
        for (uint8_t c = 0; c < 128; c += 16) {
            oled_data_buf(zeros, 16);
        }
    }
    cur_page = 0;
    cur_col  = 0;
}

void ssd1306_clear_page(uint8_t page)
{
    static uint8_t zeros[16];
    oled_set_pos(page, 0);
    for (uint8_t c = 0; c < 128; c += 16) {
        oled_data_buf(zeros, 16);
    }
    cur_page = page;
    cur_col  = 0;
}

/* =====================================================================
 * Cursor
 * ===================================================================== */
void ssd1306_set_cursor(uint8_t page, uint8_t col)
{
    oled_set_pos(page, col);
}

/* =====================================================================
 * Text normal (font 5x7, 1 pagina = 8px)
 * Fiecare caracter: 5 cols pixels + 1 col spatiu
 * ===================================================================== */
void ssd1306_putchar(char c)
{
    if (c < 0x20 || c > 0x7E) c = '?';
    uint8_t buf[6];
    for (uint8_t i = 0; i < 5; i++) {
        buf[i] = pgm_read_byte(&font5x7[(uint8_t)(c - 0x20)][i]);
    }
    buf[5] = 0x00;  /* spatiu intre caractere */
    oled_data_buf(buf, 6);
    cur_col += 6;
}

void ssd1306_puts(const char *s)
{
    while (*s) ssd1306_putchar(*s++);
}

void ssd1306_puts_at(uint8_t page, uint8_t col, const char *s)
{
    ssd1306_set_cursor(page, col);
    ssd1306_puts(s);
}

/* =====================================================================
 * Text 2x (font 10x14, 2 pagini)
 *
 * Scaling vertical 2x:
 *   Font byte b = b6..b0 (7 pixel rows).
 *   Page inferioara: expand bits 0-3 → 8 bits (fiecare bit devine 2 biti)
 *   Page superioara: expand bits 4-6 → 6 bits + 2 zero bits
 *
 * expand_nibble(b): b3b3 b2b2 b1b1 b0b0
 * ===================================================================== */
static uint8_t expand_bits(uint8_t b)
{
    uint8_t result = 0;
    for (uint8_t i = 0; i < 4; i++) {
        if (b & (1 << i)) {
            result |= (uint8_t)(3 << (i * 2));
        }
    }
    return result;
}

void ssd1306_putchar_2x(uint8_t page, uint8_t col, char c)
{
    if (c < 0x20 || c > 0x7E) c = '?';

    uint8_t lo_buf[10], hi_buf[10];

    for (uint8_t i = 0; i < 5; i++) {
        uint8_t fb = pgm_read_byte(&font5x7[(uint8_t)(c - 0x20)][i]);
        uint8_t lo = expand_bits(fb & 0x0F);           /* bits 0-3 */
        uint8_t hi = expand_bits((fb >> 4) & 0x0F);   /* bits 4-7 */
        /* Fiecare coloana de font → 2 coloane pe display (2x wide) */
        lo_buf[2*i]   = lo;
        lo_buf[2*i+1] = lo;
        hi_buf[2*i]   = hi;
        hi_buf[2*i+1] = hi;
    }

    /* Pagina inferioara */
    oled_set_pos(page, col);
    oled_data_buf(lo_buf, 10);

    /* Pagina superioara */
    oled_set_pos(page + 1, col);
    oled_data_buf(hi_buf, 10);
}

void ssd1306_puts_2x(uint8_t page, uint8_t col, const char *s)
{
    uint8_t c = col;
    while (*s) {
        ssd1306_putchar_2x(page, c, *s++);
        c += 11;   /* 10 pixels + 1 spatiu */
        if (c > 118) break;
    }
}

/* =====================================================================
 * Bara orizontala
 * ===================================================================== */
void ssd1306_draw_hbar(uint8_t page, uint8_t width, uint8_t fill_byte)
{
    oled_set_pos(page, 0);
    uint8_t buf[16];
    uint8_t col = 0;

    while (col < 128) {
        uint8_t chunk = 16;
        if (col + chunk > 128) chunk = 128 - col;
        for (uint8_t i = 0; i < chunk; i++) {
            buf[i] = (col + i < width) ? fill_byte : 0x00;
        }
        oled_data_buf(buf, chunk);
        col += chunk;
    }
}

/* =====================================================================
 * Linie verticala (marker setpoint pe bara)
 * ===================================================================== */
void ssd1306_draw_vline(uint8_t page, uint8_t col)
{
    oled_set_pos(page, col);
    oled_data(0xFF);
}

/* =====================================================================
 * Helper: afiseaza uint8 in format 2x la pozitie data
 * Maxim 3 cifre → 3 caractere × 11 = 33 cols → incape oricand
 * ===================================================================== */
void ssd1306_put_uint8_2x(uint8_t page, uint8_t col, uint8_t n)
{
    char buf[4];
    uint8_t i = 3;
    buf[3] = '\0';
    if (n == 0) {
        buf[0] = ' '; buf[1] = ' '; buf[2] = '0';
    } else {
        buf[--i] = '0' + (n % 10); n /= 10;
        buf[--i] = n ? ('0' + (n % 10)) : ' '; n /= 10;
        buf[--i] = n ? ('0' + (n % 10)) : ' ';
    }
    ssd1306_puts_2x(page, col, buf);
}

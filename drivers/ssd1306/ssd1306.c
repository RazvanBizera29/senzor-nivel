/*
 * ssd1306.c — Driver OLED SSD1306 128x64 I2C bare metal
 * Scriere directa in page mode, fara framebuffer.
 */

#include <avr/pgmspace.h>
#include "ssd1306.h"
#include "ssd1306_font.h"
#include "i2c.h"
#include "bsp.h"

#define OLED_ADDR  NANO_OLED_I2C_ADDR

/* ---- Internale ---- */
static uint8_t cur_page = 0;
static uint8_t cur_col  = 0;

static void oled_cmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_buf(OLED_ADDR, buf, 2);
}

static void oled_data_stream(const uint8_t *data, uint8_t len)
{
    i2c_start((uint8_t)((OLED_ADDR << 1) | 0));
    i2c_write(0x40);
    for (uint8_t i = 0; i < len; i++) i2c_write(data[i]);
    i2c_stop();
}

static void oled_set_pos(uint8_t page, uint8_t col)
{
    oled_cmd((uint8_t)(0xB0 | (page & 0x07)));
    oled_cmd((uint8_t)(0x00 | (col & 0x0F)));
    oled_cmd((uint8_t)(0x10 | ((col >> 4) & 0x0F)));
    cur_page = page;
    cur_col  = col;
}

/* ---- Init ---- */
void ssd1306_init(void)
{
    static const uint8_t seq[] = {
        0xAE, 0xD5,0x80, 0xA8,0x3F, 0xD3,0x00,
        0x40, 0x8D,0x14, 0x20,0x02,
        0xA1, 0xC8, 0xDA,0x12,
        0x81,0xCF, 0xD9,0xF1, 0xDB,0x40,
        0xA4, 0xA6, 0xAF
    };
    for (uint8_t i = 0; i < sizeof(seq); i++) oled_cmd(seq[i]);
    ssd1306_clear();
}

/* ---- Clear ---- */
void ssd1306_clear(void)
{
    uint8_t zeros[16] = {0};
    for (uint8_t p = 0; p < 8; p++) {
        oled_set_pos(p, 0);
        for (uint8_t c = 0; c < 128; c += 16) oled_data_stream(zeros, 16);
    }
}

void ssd1306_clear_page(uint8_t page)
{
    uint8_t zeros[16] = {0};
    oled_set_pos(page, 0);
    for (uint8_t c = 0; c < 128; c += 16) oled_data_stream(zeros, 16);
}

/* ---- Cursor ---- */
void ssd1306_set_cursor(uint8_t page, uint8_t col)
{
    oled_set_pos(page, col);
}

/* ---- Text normal ---- */
void ssd1306_putchar(char c)
{
    if (c < 0x20 || c > 0x7E) c = '?';
    uint8_t buf[6];
    for (uint8_t i = 0; i < 5; i++)
        buf[i] = pgm_read_byte(&ssd1306_font[(uint8_t)(c - 0x20)][i]);
    buf[5] = 0x00;
    oled_data_stream(buf, 6);
    cur_col = (uint8_t)(cur_col + 6);
}

void ssd1306_puts(const char *s)
{
    while (*s) ssd1306_putchar(*s++);
}

void ssd1306_puts_at(uint8_t page, uint8_t col, const char *s)
{
    oled_set_pos(page, col);
    ssd1306_puts(s);
}

/* ---- Text 2x ---- */
static uint8_t expand_bits(uint8_t b)
{
    uint8_t r = 0;
    for (uint8_t i = 0; i < 4; i++)
        if (b & (1U << i)) r |= (uint8_t)(3U << (i * 2));
    return r;
}

void ssd1306_putchar_2x(uint8_t page, uint8_t col, char c)
{
    if (c < 0x20 || c > 0x7E) c = '?';
    uint8_t lo[10], hi[10];
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t fb = pgm_read_byte(&ssd1306_font[(uint8_t)(c - 0x20)][i]);
        lo[2*i]   = lo[2*i+1] = expand_bits(fb & 0x0F);
        hi[2*i]   = hi[2*i+1] = expand_bits((fb >> 4) & 0x0F);
    }
    oled_set_pos(page, col);
    oled_data_stream(lo, 10);
    oled_set_pos((uint8_t)(page + 1), col);
    oled_data_stream(hi, 10);
}

void ssd1306_puts_2x(uint8_t page, uint8_t col, const char *s)
{
    uint8_t c = col;
    while (*s && c <= 117) {
        ssd1306_putchar_2x(page, c, *s++);
        c = (uint8_t)(c + 11);
    }
}

/* ---- Bara nivel ---- */
void ssd1306_draw_hbar(uint8_t page, uint8_t width)
{
    uint8_t buf[16];
    oled_set_pos(page, 0);
    uint8_t col = 0;
    while (col < 128) {
        uint8_t chunk = (uint8_t)(128 - col);
        if (chunk > 16) chunk = 16;
        for (uint8_t i = 0; i < chunk; i++)
            buf[i] = (col + i < width) ? 0xFF : 0x00;
        oled_data_stream(buf, chunk);
        col = (uint8_t)(col + chunk);
    }
}

/* ---- Linie verticala (marker setpoint) ---- */
void ssd1306_draw_vline(uint8_t page, uint8_t col)
{
    uint8_t b = 0xFF;
    oled_set_pos(page, col);
    oled_data_stream(&b, 1);
}

#ifndef SSD1306_H
#define SSD1306_H

/*
 * ssd1306.h — Driver OLED SSD1306 128x64, I2C, bare metal
 * Adresare: Page Mode (fara framebuffer — economie 1KB RAM)
 *
 * Layout display:
 *   Page 0 (row  0- 7): Titlu + mod
 *   Page 1 (row  8-15): Procent 2x — linie jos
 *   Page 2 (row 16-23): Procent 2x — linie sus
 *   Page 3 (row 24-31): Status pompa + alerta
 *   Page 4 (row 32-39): Bara nivel orizontala
 *   Page 5 (row 40-47): Setpoint LOW/HIGH
 */

#include <stdint.h>

void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_clear_page(uint8_t page);
void ssd1306_set_cursor(uint8_t page, uint8_t col);

/* Text normal: font 5x7, 1 pagina inaltime (6px wide cu spatiu) */
void ssd1306_putchar(char c);
void ssd1306_puts(const char *s);
void ssd1306_puts_at(uint8_t page, uint8_t col, const char *s);

/* Text 2x: font 10x14, 2 pagini inaltime */
void ssd1306_putchar_2x(uint8_t page, uint8_t col, char c);
void ssd1306_puts_2x(uint8_t page, uint8_t col, const char *s);

/* Grafice */
void ssd1306_draw_hbar(uint8_t page, uint8_t width);
void ssd1306_draw_vline(uint8_t page, uint8_t col);

#endif /* SSD1306_H */

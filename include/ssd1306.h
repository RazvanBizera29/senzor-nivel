#ifndef SSD1306_H
#define SSD1306_H

/*
 * ssd1306.h — Driver OLED SSD1306 128x64, I2C, bare metal
 * Mod adresare: Page Addressing (fara framebuffer — economie RAM)
 *
 * Layout display (8 pagini x 128 coloane):
 *   Page 0 (row  0- 7): Titlu + mod (AUTO/MANUAL)
 *   Page 1 (row  8-15): Procent mare linie sus  (font 2x)
 *   Page 2 (row 16-23): Procent mare linie jos  (font 2x)
 *   Page 3 (row 24-31): Status pompa + alerta
 *   Page 4 (row 32-39): Bara nivel (orizontala)
 *   Page 5 (row 40-47): Setpoint LOW / HIGH
 *   Page 6-7: neutilizate
 */

#include <stdint.h>

void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_clear_page(uint8_t page);

/* Pozitionare cursor */
void ssd1306_set_cursor(uint8_t page, uint8_t col);

/* Text normal (font 5x7, 1 pagina inaltie) */
void ssd1306_putchar(char c);
void ssd1306_puts(const char *s);
void ssd1306_puts_at(uint8_t page, uint8_t col, const char *s);

/* Text 2x (font 10x14, 2 pagini inaltime) */
void ssd1306_putchar_2x(uint8_t page, uint8_t col, char c);
void ssd1306_puts_2x(uint8_t page, uint8_t col, const char *s);

/* Bara orizontala: umple pagina `page` de la col 0 la `width` */
void ssd1306_draw_hbar(uint8_t page, uint8_t width, uint8_t fill_byte);

/* Linie verticala pe pagina `page` la coloana `col` */
void ssd1306_draw_vline(uint8_t page, uint8_t col);

/* Numere utilitare */
void ssd1306_put_uint8_2x(uint8_t page, uint8_t col, uint8_t n);

#endif /* SSD1306_H */

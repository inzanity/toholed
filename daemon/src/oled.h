#ifndef OLED_H
#define OLED_H

#define OLEDWIDTH 128
#define OLEDHEIGHT 64
#define SCREENBUFFERSIZE ((OLEDWIDTH/8)*OLEDHEIGHT)

#define BRIGHTNESS_HIGH 0xfff1
#define BRIGHTNESS_MED 0x4071
#define BRIGHTNESS_LOW 0x1011

typedef struct
{
    float angle;
    int base_radius;
    int hand_radius;
} analogHand;

int initOled(unsigned int level); /* Initializes OLED SSD1306 chip, brightness level arg */
int deinitOled();
void clearOled(char *screenBuffer); /* Clears screen buffer */
int updateOled(const char *screenBuffer); /* Draws screem buffer to OLED */
void drawTime(int x0, int y0, const char *tNow, char *screenBuffer); /* Draws clock to screen buffer */
void drawSmallText(int x0, int y0, const char *text, char *screenBuffer);
void drawIcon(int x0, int y0, int icon, char *screenBuffer);
int setContrastOled(unsigned int level); /* set contrast to BRIGHTNESS_HIGH _MED or _LOW */
void invertOled(bool invert); /* Select between invert and normal image */
void drawDerp(char *screenBuffer); /* Draw derp image to screen */
void drawUpdateTime(char *screenBuffer);
void drawPixel(int x, int y, int color, char *screenBuffer); /* Draw single pixel to screen */
void drawCircle(int x0, int y0, int r,  int color, char *screenBuffer); /* Draw circle to screen r=radius */
void drawLine(int x0, int y0, int x1, int y1, int color, char *screenBuffer);

void drawAnalogClock(int hours, int minutes, char *screenBuffer);
void drawHand(const analogHand hand, int color, char *screenBuffer);

#endif

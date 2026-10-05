# MicroPython: birthday message on the MakePython ESP32 built-in SSD1306 OLED
from machine import Pin, SoftI2C
import ssd1306

WIDTH, HEIGHT = 128, 64
CHAR_W = 8  # built-in framebuf font is 8x8

i2c = SoftI2C(scl=Pin(5), sda=Pin(4))
oled = ssd1306.SSD1306_I2C(WIDTH, HEIGHT, i2c)


def center(text, y):
    oled.text(text, (WIDTH - len(text) * CHAR_W) // 2, y)


oled.fill(0)
oled.rect(0, 0, WIDTH, HEIGHT, 1)
center("Happy Birthday", 12)
center("Darren!", 28)
center("* * * * *", 46)
oled.show()

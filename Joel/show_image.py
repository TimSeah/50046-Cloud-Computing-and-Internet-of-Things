# MicroPython: show photo.bin on the MakePython ESP32 built-in SSD1306 OLED
from machine import Pin, SoftI2C
import framebuf
import ssd1306

WIDTH, HEIGHT = 128, 64
IMG_SIZE = 64

i2c = SoftI2C(scl=Pin(5), sda=Pin(4))
oled = ssd1306.SSD1306_I2C(WIDTH, HEIGHT, i2c)

with open("photo.bin", "rb") as f:
    buf = bytearray(f.read())
img = framebuf.FrameBuffer(buf, IMG_SIZE, IMG_SIZE, framebuf.MONO_HLSB)

oled.fill(0)
oled.blit(img, (WIDTH - IMG_SIZE) // 2, 0)
oled.show()

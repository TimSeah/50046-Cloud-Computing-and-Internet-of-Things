# MicroPython blink for MakePython ESP32
from machine import Pin
import time

# Wiring: 3V3 -> 330 ohm -> LED (+) -> LED (-) -> IO5, so the LED is active-low
LED_ON = 0
LED_OFF = 1
INTERVAL = 0.5  # seconds

led = Pin(5, Pin.OUT, value=LED_OFF)

try:
    while True:
        led.value(LED_ON)
        time.sleep(INTERVAL)
        led.value(LED_OFF)
        time.sleep(INTERVAL)
except KeyboardInterrupt:
    led.value(LED_OFF)

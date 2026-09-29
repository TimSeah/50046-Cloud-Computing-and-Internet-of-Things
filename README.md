# 50.046 Cloud Computing and Internet of Things

Lab work for SUTD 50.046 Cloud Computing and IoT (2026).

## Labs

| Lab | Topic | Contents |
| --- | --- | --- |
| [Lab 1](Lab%201/) | Set Up the Things | Blink an external LED on a MakePython ESP32 with MicroPython |

## Lab 1: Set Up the Things

- **Hardware:** MakePython ESP32, LED, 330 Ω resistor, breadboard, USB data cable
- **Wiring:** 3V3 → 330 Ω resistor → LED long leg (+) → LED short leg (−) → IO5
- **Code:** [Lab 1/submission/main.py](Lab%201/submission/main.py) blinks IO5 every 0.5 s (active-low: `0` = on, `1` = off)
- **Handouts:** [Lab 1/docs](Lab%201/docs/)

### Running it

1. Install the Silicon Labs CP210x driver if the board has no COM port.
2. Make sure the board has MicroPython firmware (ESP32_GENERIC).
3. In Thonny, select **MicroPython (ESP32)** and the board's port, then save `main.py` to the device.

Or from the command line:

```powershell
pip install mpremote
mpremote connect COM3 fs cp "Lab 1/submission/main.py" :main.py + reset
```

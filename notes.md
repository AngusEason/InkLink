# TODO:
- Run demo code on waveshare website: https://www.waveshare.com/wiki/E-Paper_ESP32_Driver_Board#WiFi_Demo
- Compare differences in registers etc between the two screens. Is mine just not compatible with the driver baord I have?

# REPL / mpremote
## REPL
REPL stands for:
`Read – Eval – Print – Loop`
It’s an interactive programming environment where the system:

1. Reads what you type
2. Evaluates (runs) it
3. Prints the result
4. Loops back to wait for the next command

### Useful REPL commands:
- `Ctrl+B` puts it into "friendly REPL (I always want this, run on launch).
- `Ctrl+E` puts it in paste mode, allowing me to paste multi-line code with `Ctrl+V`. When done, hit `Ctrl+D` to finish and run the code.
- `Ctrl+D` reloads the board (seems the same as pressing reset)

## mpremote
mpremote is MicroPython remote control, a command line tool that provides an integrated set of utilities to remotely interact with, manage the filesystem on, and automate a MicroPython device over a serial connection. My device is actually a circuitpython device, but it works.

### Using mpremote to run REPL commands
First connect to the board using mpremote:
```
mpremote connect auto repl
```
Then type `Ctrl+B` to enter friendly REPL. Looking for >>> at the terminal line start. You can now do stuff like `print("Hello world")`

### Uploading code:
```
mpremote connect auto fs cp code.py :code.py
```

# Circuitpython web browser
Follow [this guide](https://learn.adafruit.com/circuitpython-with-esp32-quick-start/setting-up-web-workflow) to setup.
- I am hotspotting from my phone as I need a 2.4 Ghz network (Can share teh wifi, dont need to use data).
- Wifi data is stored in settings.toml:
```
CIRCUITPY_WIFI_SSID = "Angus's S24 FE"
CIRCUITPY_WIFI_PASSWORD = "youareachamp69"
CIRCUITPY_WEB_API_PASSWORD = "password"
```
- Connect to the browser with http://IP, e.g. `http://10.54.6.150/`. Password is `password` (no username)

Here you can view, read, and edit files n the **File Browser**, edit code directly using the **Full Code Editor**, and view outputs/send REPL commands using the **Serial Terminal**.

# References
- Waveshare links
    - [Driver Board](https://www.waveshare.com/wiki/E-Paper_ESP32_Driver_Board)
    - [My screen, 7.5inch e-Paper display (H)](https://www.waveshare.com/7.5inch-e-paper-hat-h.htm)
    - [wiki](https://www.waveshare.com/wiki/7.5inch_e-Paper_HAT_(H))
    - [User manual/datasheet](https://files.waveshare.com/wiki/7.5inch_e-Paper_(H)/7.5inch_e-Paper_(H).pdf)
    - [schematic](https://files.waveshare.com/upload/8/80/E-Paper_ESP32_Driver_Board_Schematic.pdf)
- [CircuitPython setup on ESP32](https://learn.adafruit.com/circuitpython-with-esp32-quick-start/installing-circuitpython)



### Board pins
import board
>>> dir(board)
['__class__', '__name__', 'D1', 'D12', 'D13', 'D14', 'D15', 'D16', 'D17', 'D18', 'D19', 'D2', 'D21', 'D22', 'D23', 'D25', 'D26', 'D27', 'D3', 'D32', 'D33', 'D34', 'D35', 'D4', 'D5', 'I2C', 'LED', 'MISO', 'MOSI', 'RX', 'RX0', 'RX2', 'SCK', 'SCL', 'SDA', 'SPI', 'TX', 'TX0', 'TX2', 'UART', 'VN', 'VP', '__dict__', 'board_id']



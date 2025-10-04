# ESP32 Reddit Display 📱

Fetches the latest post from a subreddit and displays it on an ESP32 with LCD screen.

## What It Does
- 📄 Shows latest Reddit post (title, author, score)
- 📖 Press boot button → Read full post text  
- 📜 Press boot button → Scroll through long posts
- 🔄 Auto-refreshes every 30 seconds

## Hardware
- ESP32 development board
- 1.9" ST7789 LCD display (170x320)
- USB cable

## Wiring
```
LCD → ESP32
VCC → 3.3V
GND → GND
SCL → GPIO 18
SDA → GPIO 23
RES → GPIO 4
DC  → GPIO 2
CS  → GPIO 15
BL  → GPIO 32
```

## Setup
1. **Clone this repo**
2. **Open in PlatformIO** (VS Code extension)
3. **Connect ESP32** and LCD according to wiring diagram
4. **Upload code** to ESP32
5. **Run your server** that implements the communication protocol

## Usage
1. **Power on ESP32** → Shows welcome screen
2. **Start your server** → ESP32 requests and displays latest post
3. **Press boot button once** → Switch to full text mode
4. **Press boot button again** → Scroll down through post
5. **Keep pressing** → Continue scrolling until end
6. **At end of post** → Returns to latest post mode
7. **Auto-refresh** → Gets new posts every 30 seconds (in latest mode)

## Server Communication
Any server can work as long as it:
- Listens for `REQUEST_LATEST` and `REQUEST_PLAINTEXT` commands
- Responds with `LATEST:subreddit|title|author|score` format
- Responds with `PLAINTEXT:full post content` format
- Replaces newlines `\n` with ` [NL] ` before sending

## Notes
- Scrolling is basic (will be improved)
- Works with any programming language server
- Just needs to respect the serial communication protocol

## Dependencies
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) - Display library for ESP32
- PlatformIO - Development platform

---

Built with 🫖 Berrad (Moroccan magic tool for making atay)
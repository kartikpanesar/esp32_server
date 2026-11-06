# esp32_server

A multi-client TCP chat server written in C for the **ESP32-S3**, built on **ESP-IDF** and **lwIP** sockets. It uses **event-based concurrency with `poll()`**: a single-threaded event loop serves every connected client, so there is no thread or task per connection. Messages sent by one client are relayed to the other connected clients.

## Features

- Multi-client TCP chat server running on the ESP32-S3
- Event-driven design: one loop multiplexes all sockets with `poll()`
- Low memory footprint, with no per-client thread or stack
- Message relay between connected clients
- Handles client connect and disconnect cleanly
- Reachable at a fixed IP on the LAN (DHCP reservation on the router)

## How it works

1. The server creates a listening TCP socket on port `5050`.
2. The listening socket and all client sockets are placed in a `pollfd` array.
3. The loop calls `poll()` and handles whatever is ready:
   - **Listening socket readable:** accept the new client and add it to the array.
   - **Client socket readable:** read the message and relay it to the other clients.
   - **Client closed or error:** close the socket and remove it from the array.
4. Maximum simultaneous clients: `15`. Maximum message size: `4096` bytes.

## Requirements

- ESP32-S3 development board
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/) `ESP-IDF v6.1`[, installed and exported in your shell
- A Wi-Fi network the board can join
- A TCP client such as `netcat` (or your own client)

## Build and flash

Create a wifi_config.h file and add your wifi SSID and PASSWORD as such.
```
#define SSID "wifi_name"
#define PASS "wifi_password"
```

```bash
# Set up ESP-IDF in this shell (path depends on your install)
. $HOME/esp/esp-idf/export.sh

git clone https://github.com/kartikpanesar/esp32_server.git
cd esp32_server

idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash monitor
```

Replace `<PORT>` with your serial port (for example `/dev/ttyUSB0` on Linux or `COM3` on Windows).

## Wi-Fi and fixed IP

Wifi SSID and PASSWORD live in a seperate wifi_config.h file.

To reach the board without reading its address from the serial monitor, create a **DHCP reservation** for its MAC address in your router settings. The board then gets the same IP on every boot.

## Usage

Connect from any machine on the same network:

```bash
nc <ESP32_IP> 5050
```

Open a second terminal and connect again. Anything you type in one client appears in the other.



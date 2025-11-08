# Simple TRMNL Firmware for Waveshare ESP32 EPD Driver Board

This is a simple firmware for the [Waveshare ESP32 EPD Driver Board](https://www.waveshare.com/wiki/E-Paper_ESP32_Driver_Board) that connects to a TRMNL server, fetches an image, and displays it on the e-paper display.

The current implementation supports the old [Waveshare 7.5" 640x384 e-paper display](https://www.waveshare.com/wiki/7.5inch_e-Paper_HAT) but can be easily adapted to other Waveshare displays supported by the GxEPD2 library.

Once the firmware is uploaded, the device will connect to the specified WiFi network, fetch the image from the TRMNL server, and display it on the e-paper display. It will then go to deep sleep for the configured time before repeating the process.

Some notes before you proceed:

- This firmware is purely a learning project. It’s my first dive into both embedded programming and C++, so proceed with curiosity (and caution)!
- I run the [TRMNL server](https://docs.usetrmnl.com/go/diy/byos) on a local network. If you plan to use a remote server, ensure you enable secure connections (HTTPS) on the code.

## Configuration

The firmware uses the following configuration parameters:

- `WIFI_SSID`: The SSID of the WiFi network to connect to.
- `WIFI_PASSWORD`: The password of the WiFi network.
- `TRMNL_API_URL`: The base URL of the TRMNL API.

These parameters must be defined at compile time in a `.env` file. See the `.env.example` file provided in the repository.

## Building and Uploading

To build and upload the firmware to the Waveshare ESP32 EPD Driver Board, you can use PlatformIO. Either via the Visual Studio Code extension or the PlatformIO CLI.

Just make sure to have the `.env` file with the necessary parameters in the root of the project before building. Check the `.env.example` file for reference.

If you choose to use Visual Studio Code, follow these steps:

1. Open the project in Visual Studio Code.
2. Install the [PlatformIO extension](https://platformio.org/install/ide?install=vscode) if you haven't already.
3. Connect your Waveshare ESP32 EPD Driver Board to your computer.
4. Click the "Upload" button (right arrow icon) in the PlatformIO toolbar to build and upload the firmware to the board.

If you prefer using the PlatformIO CLI, follow these steps:

1. Open a terminal and navigate to the root of the project directory.
2. Install the [PlatformIO Core CLI](https://platformio.org/install/cli) if you haven't already.
3. Connect your Waveshare ESP32 EPD Driver Board to your computer.
4. Run the command `pio run --target upload` to build and upload the firmware.

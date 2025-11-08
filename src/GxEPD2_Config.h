// NOTE: this board uses "unusual" SPI pins and requires re-mapping of HW SPI to these pins in SPIClass
//       this example shows how this can be done easily, updated for use with HSPI
//
// The Wavehare ESP32 Driver Board uses uncommon SPI pins for the FPC connector. It uses HSPI pins, but SCK and MOSI are swapped.
// To use HW SPI with the ESP32 Driver Board, HW SPI pins need be re-mapped in any case. Can be done using either HSPI or VSPI.
// Other SPI clients can either be connected to the same SPI bus as the e-paper, or to the other HW SPI bus, or through SW SPI.
// The logical configuration would be to use the e-paper connection on HSPI with re-mapped pins, and use VSPI for other SPI clients.
// VSPI with standard VSPI pins is used by the global SPI instance of the Arduino IDE ESP32 package.

// uncomment next line to use HSPI for EPD (and VSPI for SD), e.g. with Waveshare ESP32 Driver Board
#define USE_HSPI_FOR_EPD

// base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// enable or disable GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 0

// uncomment next line to use class GFX of library GFX_Root instead of Adafruit_GFX
// #include <GFX.h>
// Note: if you use this with ENABLE_GxEPD2_GFX 1:
//       uncomment it in GxEPD2_GFX.h too, or add #include <GFX.h> before any #include <GxEPD2_GFX.h>

#include <GxEPD2_BW.h>

// select the display class (only one), matching the kind of display panel
#define GxEPD2_DISPLAY_CLASS GxEPD2_BW

// select the display driver class (only one) for your  panel
#define GxEPD2_DRIVER_CLASS GxEPD2_750 // GDEW075T8   640x384, UC8159c (IL0371), (WF0583CZ09)

// somehow there should be an easier way to do this
#define GxEPD2_BW_IS_GxEPD2_BW true
// #define GxEPD2_3C_IS_GxEPD2_3C true
// #define GxEPD2_7C_IS_GxEPD2_7C true
// #define GxEPD2_1248_IS_GxEPD2_1248 true
#define IS_GxEPD(c, x) (c##x)
#define IS_GxEPD2_BW(x) IS_GxEPD(GxEPD2_BW_IS_, x)
// #define IS_GxEPD2_3C(x) IS_GxEPD(GxEPD2_3C_IS_, x)
// #define IS_GxEPD2_7C(x) IS_GxEPD(GxEPD2_7C_IS_, x)
// #define IS_GxEPD2_1248(x) IS_GxEPD(GxEPD2_1248_IS_, x)

#if defined(ESP32)
#define MAX_DISPLAY_BUFFER_SIZE 65536ul // e.g.
#if IS_GxEPD2_BW(GxEPD2_DISPLAY_CLASS)
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))
#elif IS_GxEPD2_3C(GxEPD2_DISPLAY_CLASS)
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8))
#elif IS_GxEPD2_7C(GxEPD2_DISPLAY_CLASS)
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE) / (EPD::WIDTH / 2) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE) / (EPD::WIDTH / 2))
#endif
GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=*/15, /*DC=*/27, /*RST=*/26, /*BUSY=*/25));
#endif

#if defined(ESP32) && defined(USE_HSPI_FOR_EPD)
SPIClass hspi(HSPI);
#endif

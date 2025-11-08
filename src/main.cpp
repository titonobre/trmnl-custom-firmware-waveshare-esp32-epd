#include <Arduino.h>
#include <ArduinoJson.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include <PNGdec.h>
#include <LittleFS.h>

#include <GxEPD2_Config.h>

// WiFi credentials

String wifiSsid = WIFI_SSID;
String wifiPassword = WIFI_PASSWORD;

// TRMNL Stuff

String trmnlApiUrl = TRMNL_API_URL;
String trmnlApiKey;

// Current Screen

String currentScreenImageURL;
String currentScreenLocalFilePath = "/screen.png";
int currentScreenSleepTime = 15 * 60; // in seconds, default to 15 minutes

// PNG Decoding

PNG png;

// Set time via NTP, as required for x.509 validation
void updateClock()
{
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync...");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.printf("Current time: %s\n", asctime(&timeinfo));
}

// PNG draw callback - called for each line of the image
int pngDraw(PNGDRAW *pDraw)
{
  uint16_t y = pDraw->y;
  uint8_t *line = pDraw->pPixels;

  // For 1-bit PNG, pixels are already packed (8 pixels per byte)
  // GxEPD2 expects white = 1, black = 0
  // PNG typically has black = 0, white = 1, so we may need to invert

  for (uint16_t x = 0; x < pDraw->iWidth; x++)
  {
    uint8_t byteIndex = x / 8;
    uint8_t bitIndex = 7 - (x % 8);
    uint8_t pixel = (line[byteIndex] >> bitIndex) & 0x01;

    // Draw pixel to display
    // If colors are inverted, use: pixel = !pixel
    display.drawPixel(x, y, pixel ? GxEPD_WHITE : GxEPD_BLACK);
  }

  return 1; // Return 1 to continue decoding
}

// File open callback for PNGdec
void *pngFileOpen(const char *filename, int32_t *size)
{
  File *f = new File(LittleFS.open(filename, "r"));
  if (f)
  {
    *size = f->size();
    return (void *)f;
  }
  return NULL;
}

// File close callback
void pngFileClose(void *handle)
{
  File *f = (File *)handle;
  if (f)
  {
    f->close();
    delete f;
  }
}

// File read callback
int32_t pngFileRead(PNGFILE *handle, uint8_t *buffer, int32_t length)
{
  File *f = (File *)handle->fHandle;
  if (f && f->available())
  {
    return f->read(buffer, length);
  }
  return 0;
}

// File seek callback
int32_t pngFileSeek(PNGFILE *handle, int32_t position)
{
  File *f = (File *)handle->fHandle;
  if (f)
  {
    return f->seek(position);
  }
  return 0;
}

bool loadPNGFromFile(const char *filename)
{
  int rc;

  // Open PNG from filesystem with custom callbacks
  rc = png.open(filename, pngFileOpen, pngFileClose, pngFileRead, pngFileSeek, pngDraw);

  if (rc == PNG_SUCCESS)
  {
    Serial.printf("Image specs: (%d x %d), %d bpp, pixel type: %d\n",
                  png.getWidth(), png.getHeight(),
                  png.getBpp(), png.getPixelType());

    // Decode the image
    rc = png.decode(NULL, 0);
    png.close();

    if (rc == PNG_SUCCESS)
    {
      Serial.println("PNG decode successful");
      return true;
    }
    else
    {
      Serial.printf("PNG decode failed: %d\n", rc);
      return false;
    }
  }
  else
  {
    Serial.printf("PNG open failed: %d\n", rc);
    return false;
  }
}

// update the screen with the fetched image
void updateScreen()
{
  Serial.println("Updating Screen...");

  // decode and display from file
  display.setFullWindow();
  display.firstPage();

  do
  {
    display.fillScreen(GxEPD_WHITE);

    // Load and draw PNG from file
    if (!loadPNGFromFile(currentScreenLocalFilePath.c_str()))
    {
      Serial.println("Failed to load PNG");
    }

  } while (display.nextPage());
}

// download file from URL to filesystem
void downloadFile(const String url, const String filepath)
{
  Serial.printf("Downloading file %s\n", url.c_str());

  // Delete existing file
  if (LittleFS.exists(filepath))
  {
    LittleFS.remove(filepath);
  }

  // Create file for writing
  File file = LittleFS.open(filepath, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure(); // disable certificate verification

  HTTPClient https;

  https.begin(client, url);
  https.setTimeout(15000);

  int responseCode = https.GET();

  // handle connection errors
  if (responseCode < 0)
  {
    Serial.println("Connection failed!");
    Serial.printf("Message: %s\n", https.errorToString(responseCode).c_str());
    https.end();
    client.stop();
    return;
  }

  // handle HTTP request non-200 responses
  if (responseCode != HTTP_CODE_OK)
  {
    Serial.printf("HTTP request failed with code: %d\n", responseCode);
    https.end();
    client.stop();
    return;
  }

  https.writeToStream(&file);

  file.close();
  https.end();
  client.stop();
}

// fetch display info from TRMNL
void fetchTrmnlDisplay()
{
  Serial.println("Fetching TRMNL display...");

  WiFiClientSecure client;
  client.setInsecure(); // disable certificate verification

  HTTPClient https;

  https.begin(client, trmnlApiUrl + "/display");
  https.addHeader("ID", WiFi.macAddress());
  https.addHeader("Access-Token", trmnlApiKey);
  https.addHeader("RSSI", String(WiFi.RSSI()));

  int responseCode = https.GET();

  // handle connection errors
  if (responseCode < 0)
  {
    Serial.println("Connection failed!");
    Serial.printf("Message: %s\n", https.errorToString(responseCode).c_str());
    https.end();
    client.stop();
    return;
  }

  // handle HTTP request non-200 responses
  if (responseCode != HTTP_CODE_OK)
  {
    Serial.printf("HTTP request failed with code: %d\n", responseCode);
    https.end();
    client.stop();
    return;
  }

  // Parse JSON response
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, https.getStream());

  if (error)
  {
    Serial.printf("Failed to parse JSON response: %s\n", error.f_str());
    https.end();
    client.stop();
    return;
  }

  https.end();
  client.stop();

  currentScreenImageURL = "https://" + doc["image_url"].as<String>();
  currentScreenSleepTime = doc["refresh_rate"].as<int>();
}

// fetch the next screen image from TRMNL
void fetchNextScreen()
{
  // fetch display info
  fetchTrmnlDisplay();

  // download to file
  downloadFile(currentScreenImageURL, currentScreenLocalFilePath);
}

// initialize trmnl device
void initTrmnlDevice()
{
  Serial.println("Initializing TRMNL device...");

  WiFiClientSecure client;
  client.setInsecure(); // disable certificate verification

  HTTPClient https;

  https.begin(client, trmnlApiUrl + "/setup");
  https.addHeader("ID", WiFi.macAddress());

  int responseCode = https.GET();

  // handle connection errors
  if (responseCode < 0)
  {
    Serial.println("Connection failed!");
    Serial.printf("Message: %s\n", https.errorToString(responseCode).c_str());
    https.end();
    client.stop();
    return;
  }

  // handle HTTP request non-200 responses
  if (responseCode != HTTP_CODE_OK)
  {
    Serial.printf("HTTP request failed with code: %d\n", responseCode);
    https.end();
    client.stop();
    return;
  }

  // Parse JSON response
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, https.getStream());

  if (error)
  {
    Serial.printf("Failed to parse JSON response: %s\n", error.f_str());
    https.end();
    client.stop();
    return;
  }

  https.end();
  client.stop();

  trmnlApiKey = doc["TRMNL_API_URL_key"].as<String>();
}

// Connect to WiFi
void connectToWiFi()
{
  Serial.print("Connecting to WiFi...");
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.begin(wifiSsid, wifiPassword);

  int ConnectTimeout = 60; // 30 seconds

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    Serial.print(WiFi.status());
    if (--ConnectTimeout <= 0)
    {
      Serial.println();
      Serial.println("WiFi Connect Timeout");
      return;
    }
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.println(WiFi.localIP());
}

// initialize the filesystem
void initFilesystem()
{
  Serial.println("Initializing LittleFS...");

  if (!LittleFS.begin(true))
  {
    Serial.println("LittleFS Mount Failed");
    return;
  }
}

// initialize the display
void initDisplay()
{
// special handling for Waveshare ESP32 Driver board
#if defined(ESP32) && defined(USE_HSPI_FOR_EPD)
  hspi.begin(13, 12, 14, 15); // remap hspi for EPD (swap pins)
  display.epd2.selectSPI(hspi, SPISettings(4000000, MSBFIRST, SPI_MODE0));
#endif

  display.init(115200);
}

// turn off display to increase longevity and save power if running on battery
void powerOffDisplay()
{
  display.powerOff();
}

// enable deep sleep
void goToSleep()
{
  Serial.printf("Going to deep sleep for %d seconds...\n", currentScreenSleepTime);
  esp_sleep_enable_timer_wakeup(currentScreenSleepTime * 1000000ULL);
  esp_deep_sleep_start();
}

void setup()
{
  Serial.begin(115200);

  initFilesystem();
  connectToWiFi();
  updateClock();
  initTrmnlDevice();
  fetchNextScreen();
  initDisplay();
  updateScreen();
  powerOffDisplay();
  goToSleep();
}

void loop()
{
}

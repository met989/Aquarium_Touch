#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>

// ==============================================================================
//                   CENTRAL CONFIGURATION & PIN DEFINITIONS
//                (Single Point of Truth for the entire project)
// ==============================================================================

// ------------------------------------------------------------------------------
// 1. TFT_eSPI Display Hardware & Driver Configuration
// ------------------------------------------------------------------------------
#ifndef USER_SETUP_LOADED
#define USER_SETUP_LOADED 1
#endif

#define ILI9341_DRIVER 1
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_OFF 1
#define TFT_WIDTH 240
#define TFT_HEIGHT 320

// Display SPI Pins
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST -1
#define TFT_BL 21
#define TFT_BACKLIGHT_ON HIGH

// ------------------------------------------------------------------------------
// 2. Touchscreen (XPT2046) Pins
// ------------------------------------------------------------------------------
#define TOUCH_CS 33
#define TOUCH_IRQ 36
#define TOUCH_MOSI 32
#define TOUCH_MISO 39
#define TOUCH_SCK 25

// ------------------------------------------------------------------------------
// 3. SD Card, Sensors & Output Pins
// ------------------------------------------------------------------------------
#define SD_CS 5
#define BOOT_BTN 0
#define TEMP_SENSOR_PIN 4  // DS18B20 1-Wire Data Pin
#define LIGHT_RELAY_PIN 17 // Aquarium Light Relay / MOSFET Output Pin

// ------------------------------------------------------------------------------
// 4. TFT_eSPI Fonts & SPI Speeds
// ------------------------------------------------------------------------------
#define LOAD_GLCD 1
#define LOAD_FONT2 1
#define LOAD_FONT4 1
#define LOAD_FONT6 1
#define LOAD_FONT7 1
#define LOAD_FONT8 1
#define LOAD_GFXFF 1
#define SMOOTH_FONT 1

#define SPI_FREQUENCY 27000000
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000
#define USE_HSPI_PORT 1

// ------------------------------------------------------------------------------
// 5. Colors (RGB565)
// ------------------------------------------------------------------------------
#define COLOR_BG_OCEAN 0x08A7      // Sfondo principale delle schermate/tab (Blu Oceano scuro)
#define COLOR_OCEAN_DEPTH 0x0048   // Sfondo dei caricamenti/formattazione (Blu Abisso profondo)
#define COLOR_WATER_TOP 0x0A55     // Colore delle onde animate in alto sulla Dashboard (Azzurro onda)
#define COLOR_CARD_BG 0x11EC       // Sfondo semitrasparente dei pannelli/pulsanti e tastiera (Blu Vetro)
#define COLOR_CARD_BORDER 0x2C18   // Bordo predefinito dei pannelli in vetro (Grigio/Blu scuro)
#define COLOR_CYAN_GLOW 0x07FF     // Colore di evidenza Neon per testi, valori di temperatura e bordi attivi (Ciano)
#define COLOR_EMERALD_GREEN 0x0760 // Colore per stati attivi/sicuri (es. Temperatura OK, Connesso, pulsante SI) (Verde)
#define COLOR_CORAL_RED 0xF965     // Colore per stati di avviso/errore (es. Temp. alta, Disconnesso, pulsante NO) (Rosso)
#define COLOR_GOLD_ACCENT 0xFEA0   // Dettagli dorati come icone attive della barra di navigazione e testi secondari (Oro)
#define COLOR_MOON_BLUE 0x3DFE     // Colore per indicatori disattivi (es. Cerchio con luce spenta) (Blu Luna)
#define COLOR_TEXT_MUTED 0x8C71    // Colore per etichette secondarie e descrizioni testuali (Grigio chiaro)
#define COLOR_NAV_BG 0x0845        // Sfondo fisso della barra di navigazione in basso (Nero/Blu scuro)

// ------------------------------------------------------------------------------
// 6. Optimal Temperature Target Configuration
// ------------------------------------------------------------------------------
#define DEFAULT_TEMP_MIN 24.0f // °C Default minimum optimal temperature
#define DEFAULT_TEMP_MAX 27.0f // °C Default maximum optimal temperature
#define TEMP_HYSTERESIS 0.3f   // °C Hysteresis band

// ------------------------------------------------------------------------------
// 7. System Constants & File Paths
// ------------------------------------------------------------------------------
#define AQUARIUM_OS_VERSION "0.5-10082026"

static const uint32_t SERIAL_BAUD = 115200;
static const char *CONFIG_PATH = "/config.cfg";
static const char *CONFIG_TMP_PATH = "/config.tmp";
static const char *CONFIG_BAK_PATH = "/config.bak";
static const uint32_t BOOT_HOLD_MS = 3000;

// Date Format Options
#define DATE_FORMAT_DDMMYYYY 0 // DD/MM/YYYY
#define DATE_FORMAT_MMDDYYYY 1 // MM/DD/YYYY
#define DATE_FORMAT_YYYYMMDD 2 // YYYY/MM/DD

#define DEFAULT_DATE_FORMAT DATE_FORMAT_DDMMYYYY

// Language, NTP & Timezone Configuration Defaults
#define DEFAULT_LANGUAGE_FILE "english.lng"
#define DEFAULT_LANGUAGE_NAME "English"
#define LANGUAGES_DIR "/languages"
#define DEFAULT_TIMEZONE "0"
#define DEFAULT_NTP_SERVER1 "pool.ntp.org"
#define DEFAULT_NTP_SERVER2 "time.nist.gov"

// ------------------------------------------------------------------------------
// 8. UI & Animation Configuration
// ------------------------------------------------------------------------------
#define UI_SPRITE_WIDTH 320  // Full screen width in pixels
#define UI_SPRITE_HEIGHT 172 // Main content height in pixels (110 KB heap safe)
#define UI_SPRITE_Y_OFFSET                                                     \
  4 // Top Y position of main content sprite (tiny 4px top margin)
#define UI_NAVBAR_Y 190 // Top Y position of bottom navigation bar
#define UI_NAVBAR_HEIGHT                                                       \
  50 // Height of bottom navigation bar in pixels (reduced ~25%)
#define UI_DASHBOARD_ANIM_FPS 30 // Target FPS for Dashboard animations
#define UI_ANIM_UPDATE_INTERVAL_MS (1000 / UI_DASHBOARD_ANIM_FPS)
#define UI_STATIC_REFRESH_MS 200 // Refresh interval for static screens
#define UI_BUBBLE_COUNT 14       // Number of background water bubbles
#define UI_FISH_COUNT 12         // Number of swimming fish
#define UI_TOUCH_DEBOUNCE_MS 250 // Touch debounce in milliseconds
// ------------------------------------------------------------------------------
// 9. Runtime Configuration Structs (shared between main.cpp and aquarium_server.cpp)
// ------------------------------------------------------------------------------

#ifdef __cplusplus
struct TouchCal {
  int minX, maxX, minY, maxY;
};

struct AppConfig {
  uint8_t screenMode;
  String wifiSsid, wifiPassword;
  String wifiIpStatic, wifiSubnet, wifiGateway, wifiDns1, wifiDns2;
  String weatherApiKey, weatherCity;
  String ntpServer1, ntpServer2, timezone;
  TouchCal touch[4];   // Screen rotation calibration (0..3)
  String mqttServer;
  int mqttPort;
  String mqttUser, mqttPassword;
  int formatHour;
  bool debug;
};

extern AppConfig cfg;
extern SemaphoreHandle_t g_configMutex;
extern volatile bool g_saveConfigNeeded;

#define DBG_PRINT(x)                                                           \
  do {                                                                         \
    if (cfg.debug)                                                             \
      Serial.print(x);                                                         \
  } while (0)
#define DBG_PRINTLN(x)                                                         \
  do {                                                                         \
    if (cfg.debug)                                                             \
      Serial.println(x);                                                       \
  } while (0)
#define DBG_PRINTF(...)                                                        \
  do {                                                                         \
    if (cfg.debug)                                                             \
      Serial.printf(__VA_ARGS__);                                              \
  } while (0)

#endif


#endif // APP_CONFIG_H

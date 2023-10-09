/************************ Adafruit IO Config *******************************/

#define IO_USERNAME  "partyBot_Inc"
#define IO_KEY       "aio_qesc82aQSK8aWx6T7Au5Fce3XVbt"

/******************************* WIFI **************************************/

#define WIFI_SSID "<insert ID>"
#define WIFI_PASS "<insert password>"

#include "AdafruitIO_WiFi.h"

#if defined(USE_AIRLIFT) || defined(ADAFRUIT_METRO_M4_AIRLIFT_LITE) ||         \
    defined(ADAFRUIT_PYPORTAL)

// Configure the pins used for the ESP32 connection
#if !defined(SPIWIFI_SS)

#define SPIWIFI SPI
#define SPIWIFI_SS 10 // Chip select pin
#define NINA_ACK 9    // a.k.a BUSY or READY pin
#define NINA_RESETN 6 // Reset pin
#define NINA_GPIO0 -1 // Not connected
#endif

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS, SPIWIFI_SS,
                   NINA_ACK, NINA_RESETN, NINA_GPIO0, &SPIWIFI);
#else
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
#endif
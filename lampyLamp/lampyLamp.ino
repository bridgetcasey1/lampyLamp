#include "config.h"
#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 8
#define LED_PIN 5  // D1 = GPIO5
#define TOUCH_PIN 12 // D6 = GPIO12
#define LAMP_1_ID 27
#define LAMP_2_ID 100
#define MAX_RAINBOW_CYCLES 3
#define MAX_RAINBOW_SPINS 5

// Colours
#define OFF 0
#define WHITE 1
#define PINK 2
#define RED 3
#define ORANGE 4
#define YELLOW 5
#define GREEN 6
#define LIGHT_BLUE 7
#define DARK_BLUE 8
#define PURPLE 9
#define RAINBOW 10
#define RAINBOW_SPIN 11
#define SLOW_FLASH 12
#define FAST_FLASH 13
#define NO_MODE 14

#define LED_BRIGHTNESS 50  // max = 255
#define RAINBOW_MODE_1 1
#define RAINBOW_MODE_2 2
#define FLASH_MODE_1  1
#define FLASH_MODE_2  2

//////////////////LAMP ID////////////////////////////////////////////////////////////
int lampID = LAMP_1_ID;
/////////////////////////////////////////////////////////////////////////////////////

// Set up the colour feed
AdafruitIO_Feed *lamp = io.feed("lampyLamp");

// Define LED strip
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Track selected colour
int selectedColour = OFF; 
int currentMode = NO_MODE; 

// Track touch sensor status
int tapped = 0;

// Value that should activate the lamp - code will figure out what this should be in the setup
//int recVal = 0;
//int listening = 0;

// Timer for detecting double tap
unsigned long lastTapTime = 0; 
int doubleTapMode = RAINBOW_MODE_2; 

// Variables for playing out rainbow pattern (two modes)
unsigned long previousMillis = 0;
unsigned long interval = 2500;
int rainbowPlaying = 0; 
int rainbowSpin = 0; 
int rainbowCycles = 0; 

// Variables for flashing colour
int flashColour = 0; 
int slowFlash = 0;
int colourOn = 0; 
int increasing = 0; 
int flashBrightness = LED_BRIGHTNESS;
int flashMode = FLASH_MODE_1; 

// Timer to detect inactivity
unsigned long lastColourChangeTime = 0; 
unsigned long inactivityInterval = 3600000;  // 1 hour

void setup() {

  // Setup serial connection
  Serial.begin(9600);
  while(! Serial);    // Wait for serial monitor to open

  /* Set lamp receive values
  if (lampID == LAMP_1_ID)  {
    recVal = LAMP_2_ID;
  } else {
    recVal = LAMP_1_ID;
  }*/

  // Setup LED strip
  strip.begin();    // Initialise NeoPixel strip object
  strip.show();    // Turn off all pixels
  strip.setBrightness(LED_BRIGHTNESS);    

  // Setup touch as input and interrupt
  pinMode(TOUCH_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(TOUCH_PIN), tapDetected, CHANGE);
  
  // Connect to io.adafruit.com
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  // Get the status of the value in Adafruit IO
  lamp->onMessage(handleMessage);

  // Wait for a connection - play the "spin" Neopixel animation while connecting
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    spin();
    delay(500);
  }

  // Connected
  Serial.println();
  Serial.println(io.statusText());
  flash();

  // Get the status of our value in Adafruit IO
  lamp->get();
  
  previousMillis = millis();

  //lamp->save(lampID); 
}

void loop() {

  // Keep the ESP8266 connected to Adafruit IO
  io.run();

  // Play out rainbow as appropriate
  if (rainbowPlaying == 1) {
    // Update to next colour after sufficient delay
    unsigned long currentMillis = millis();
    if ((currentMillis - previousMillis) > interval) {
      // Update lamp colour
      selectedColour++;
      if (selectedColour == 10) {
        rainbowCycles++;
        if (rainbowCycles >= MAX_RAINBOW_CYCLES) {
          rainbowPlaying = 0; 
          selectedColour = WHITE;
        } else {
          selectedColour = RED;
        }
      }
      setColour(selectedColour);

      // Update other lamp
      //lamp->save(lampID); 
      //lamp->save(selectedColour);
      previousMillis = currentMillis; 
    }
  } else if (rainbowSpin == 1) {
    // Play next rainbow spin after sufficient delay
    unsigned long currentMillis = millis();
    if ((currentMillis - previousMillis) > interval) {
      // Rainbow spin
      for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
        for(int i=0; i < strip.numPixels(); i++) {
          int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
          strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
        }
      strip.show();
      }
      rainbowCycles++;
      if (rainbowCycles >= MAX_RAINBOW_SPINS) {
          rainbowSpin = 0; 
      }
      previousMillis = currentMillis; 
    }
  } else if (flashColour == 1) {
    // Turn on/off lamp after sufficient delay
    unsigned long currentMillis = millis();
    if ((currentMillis - previousMillis) > 1000) {
      if (colourOn == 0) {
        setColour(selectedColour);
        // Update other lamp
        //lamp->save(lampID); 
        //lamp->save(selectedColour);
        colourOn = 1; 
      } else {
        strip.clear(); 
        strip.show();
        // Update other lamp
        //lamp->save(lampID); 
        //lamp->save(OFF);
        colourOn = 0; 
      }
      previousMillis = currentMillis; 
    } 
  } else if (slowFlash == 1) {
    // Increase/decrease brightness after sufficient delay
    unsigned long currentMillis = millis();
    if ((currentMillis - previousMillis) > 2) {
      if (increasing == 1) {
        if (flashBrightness < 100) {
          flashBrightness++; 
        } else {
          increasing = 0; 
          flashBrightness--; 
        }
      } else {
        if (flashBrightness > 1) {
          flashBrightness--; 
        } else {
          increasing = 1; 
          flashBrightness++; 
        }
      }
      strip.setBrightness(flashBrightness);
      strip.show(); 
      previousMillis = currentMillis; 
    } 
  }

  // Detect inactivity - turn off if so
  unsigned long currentMillis = millis();
  if ((currentMillis - lastColourChangeTime) > inactivityInterval) {
    strip.clear();
    strip.show();
  }
}

// Create blue spinning animation
void spin() {
  for(int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, 0, 0, 255);
      strip.show();
      delay(20);
    }
    for(int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, 0, 0, 0);
      strip.show();
      delay(20);
    }
}

// Code that tells the ESP8266 what to do when it recieves new data from the Adafruit IO feed
void handleMessage(AdafruitIO_Data *data) {

  // Convert the recieved data to an int
  int reading = data->toInt();

  // Check if message is from other lamp 
  /*if (reading == recVal) {
    listening = 1; 
  } else if (reading == lampID) {
    listening = 0; 
  } else if (listening == 1) {*/
    // Turn off rainbow/flash modes if on
  
  // Only process message if different from current mode/colour
  if (reading == currentMode or reading == selectedColour) {
    // Do nothing
  } else {
    rainbowPlaying = 0; 
    rainbowSpin = 0; 
    flashColour = 0;
    slowFlash = 0;
    strip.setBrightness(LED_BRIGHTNESS);  

    if (reading < RAINBOW) {
      currentMode = NO_MODE; 
    }

    setColour(reading); 
  }
  //listening = 0; 
  //}
}

// Code to flash the Neopixels when a stable connection to Adafruit IO is made
void flash() {

  for(int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, 255, 255, 255);
    }
  strip.show();
  
  delay(200);

  for(int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, 0, 0, 0);
    }
  strip.show();
  
  delay(200);

  for(int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, 255, 255, 255);
    }
  strip.show();
  
  delay(200);

  for (int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, 0, 0, 0);
    }
  strip.show();
  
  delay(200);
}

// Set pixel colours
void setColour(int colour) {
  int oldColour = selectedColour;
  selectedColour = colour;
  switch(colour) {
    case OFF:
      strip.clear(); 
      strip.show();
      break; 
    case WHITE:
      setPixels(strip.Color(255, 255, 255));
      break;
    case PINK:
      setPixels(strip.Color(242, 90, 255));
      break;
    case RED:
      setPixels(strip.Color(255, 0, 0));
      break;
    case ORANGE:
      setPixels(strip.Color(255, 40, 0));
      break;
    case YELLOW:
      setPixels(strip.Color(255, 150, 0));
      break;
    case GREEN:
      setPixels(strip.Color(0, 255, 0));
      break;
    case LIGHT_BLUE:
      setPixels(strip.Color(50, 255, 255));
      break;
    case DARK_BLUE:
      setPixels(strip.Color(0, 0, 255));
      break;
    case PURPLE:
      setPixels(strip.Color(180, 0, 255));
      break; 
    case RAINBOW:
      rainbowPlaying = 1; 
       rainbowCycles = -1; 
       doubleTapMode = RAINBOW_MODE_1; 
       // Update current colour to purple to start on red
       selectedColour = PURPLE;
       currentMode = RAINBOW;
       break;
    case RAINBOW_SPIN:
      selectedColour = oldColour; 
      rainbowSpin = 1; 
      rainbowCycles = 0;
      currentMode = RAINBOW_SPIN;
      break;
    case SLOW_FLASH:
      selectedColour = oldColour; 
      slowFlash = 1; 
      increasing = 0; 
      flashBrightness = LED_BRIGHTNESS;
      currentMode = SLOW_FLASH;
      break; 
    case FAST_FLASH:
      flashColour = 1;
      colourOn = 1; 
      flashMode = FLASH_MODE_1;
      currentMode = FAST_FLASH;
      break; 
    default:
      selectedColour = OFF;
      break; 
  }
  lastColourChangeTime = millis(); 
}

// Sends pixel colour to LED pixels
void setPixels(uint32_t colour) {
  for(int i=0; i < strip.numPixels(); i++) { 
    strip.setPixelColor(i, colour);         
    strip.show();                          
  }
}

// ISR that runs when the touch sensor is activated
ICACHE_RAM_ATTR void tapDetected() {

  // Only process tap (not release)
  if (tapped == 0) {

    // Check for double tap
    int doubleTap = 0; 
    unsigned long currentTime = millis(); 
    if ((currentTime - lastTapTime) < 200) {
      doubleTap = 1;
    }

    // Turn off rainbow/flash modes if on
    rainbowPlaying = 0; 
    rainbowSpin = 0; 
    flashColour = 0;
    slowFlash = 0;
    currentMode = NO_MODE;
    strip.setBrightness(LED_BRIGHTNESS);  

    if (doubleTap == 0) {
      // Update colour
      selectedColour++;
      if (selectedColour == 10) {
        selectedColour = OFF;
      }
      setColour(selectedColour);

      // Update other lamp
      //lamp->save(lampID); 
      lamp->save(selectedColour);
    } else {
      if (doubleTapMode == RAINBOW_MODE_2) {
        rainbowPlaying = 1; 
        rainbowCycles = -1; 
        doubleTapMode = RAINBOW_MODE_1; 
        currentMode = RAINBOW;
        // Update current colour to purple to start on red
        selectedColour = PURPLE;
        lamp->save(RAINBOW);  // Update other lamp
      } else {
        rainbowSpin = 1; 
        rainbowCycles = 0;
        doubleTapMode = RAINBOW_MODE_2;
        currentMode = RAINBOW_SPIN;
        //lamp->save(lampID); 
        lamp->save(RAINBOW_SPIN);  // Update other lamp
      }
    }
    tapped = 1;
    lastTapTime = millis(); 
  } else {
    // Check for long hold
    unsigned long currentTime = millis(); 
    if ((currentTime - lastTapTime) > 750) {
      if (flashMode == FLASH_MODE_1) {
        slowFlash = 1; 
        increasing = 0; 
        flashBrightness = LED_BRIGHTNESS;
        flashMode = FLASH_MODE_2;
        currentMode = SLOW_FLASH;
        //lamp->save(lampID); 
        lamp->save(SLOW_FLASH);  // Update other lamp    
      } else {
        flashColour = 1;
        colourOn = 1; 
        flashMode = FLASH_MODE_1; 
        currentMode = FAST_FLASH;
        lamp->save(FAST_FLASH);  // Update other lamp
      }
    }
    tapped = 0; 
  }
}
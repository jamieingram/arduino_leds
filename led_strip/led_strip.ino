#include <LPD8806.h>
#include <SPI.h>
#include <Ethernet.h>
#include <math.h>
#include <PusherClient.h>

#define DEBUG //Comment out to turn DEBUG mode OFF
#include <DebugUtils.h>

//Pusher config
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
PusherClient client;

// Number of RGB LEDs in strand:
byte nLEDs = 32;
byte colours[96];

// Chose 2 pins for output; can be any valid output pins:
byte dataPin  = 2;
byte clockPin = 3;

LPD8806 strip = LPD8806(nLEDs, dataPin, clockPin);

void setup() {
  #ifdef DEBUG
  Serial.begin(9600);
#endif
  DEBUG_PRINT_LN("Starting up...");
  if (Ethernet.begin(mac) == 0) {
    DEBUG_PRINT_LN("Init Ethernet failed");
  }
  //
  //connect the websocket
  if(client.connect("c414e66e67cf1233b872")) {
    DEBUG_PRINT_LN("Connected...");
    client.bind("update", updateLights);
    client.subscribe("status");
  }
  // Start up the LED strip
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();
  //
  colorChase(strip.Color(127, 127, 127), 50); // White
}

void loop() {
  if (client.connected()) {
    client.monitor();
  }
  setLightColours();
}

void updateLights(String data) {
  data = data.substring(data.indexOf('data\":\"\\') + 2,data.length());
  data = data.substring(0, data.indexOf('\\'));
  DEBUG_PRINT_LN(data);
  //parse the data and store the current values
  int comma_pos;
  byte i, colour_val, array_pos;

  if (data.length() > 0) {
    for (i = 0 ; i < sizeof(colours) ; i++) colours[i] = 0;
    array_pos = 0;
  }

  do
  {
    comma_pos = data.indexOf(',');
    if(comma_pos != -1) {
      colour_val = (byte) (data.substring(0,comma_pos).toInt() / 2);
      colours[array_pos] = floor(colour_val);
      data = data.substring(comma_pos+1, data.length());
      array_pos++;
    } else {  // here after the last comma is found
      if (data.length() > 0) {
        colour_val = (byte) (data.toInt() / 2);
        colours[array_pos] = floor(colour_val);
      }
    }
  } while (comma_pos >= 0);
  //
}

void setLightColours() {
  int i;
  uint32_t colour;
  byte r, g, b;
  for (i=0; i < strip.numPixels(); i++) {
    r = colours[3*i];
    g = colours[(3*i)+1];
    b = colours[(3*i)+2];
    colour = strip.Color(r, g, b);
    strip.setPixelColor(i,colour);
  }
  strip.show();
}

void rainbow(uint8_t wait) {
  int i, j;
   
  for (j=0; j < 384; j++) {     // 3 cycles of all 384 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel( (i + j) % 384));
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
  
  for (j=0; j < 384 * 5; j++) {     // 5 cycles of all 384 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color wheel
      // (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel( ((i * 384 / strip.numPixels()) + j) % 384) );
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

//Input a value 0 to 384 to get a color value.
//The colours are a transition r - g -b - back to r

uint32_t Wheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128;//Red down
      g = WheelPos % 128;     // Green up
      b = 0;                  //blue off
      break; 
    case 1:
      g = 127 - WheelPos % 128;  //green down
      b = WheelPos % 128;      //blue up
      r = 0;                  //red off
      break; 
    case 2:
      b = 127 - WheelPos % 128;  //blue down 
      r = WheelPos % 128;      //red up
      g = 0;                  //green off
      break; 
  }
  return(strip.Color(r,g,b));
}

// Chase one dot down the full strip.
void colorChase(uint32_t c, uint8_t wait) {
  int i;

  // Start by turning all pixels off:
  for(i=0; i<strip.numPixels(); i++) strip.setPixelColor(i, 0);

  // Then display one pixel at a time:
  for(i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c); // Set new pixel 'on'
    strip.show();              // Refresh LED states
    strip.setPixelColor(i, 0); // Erase pixel, but don't refresh!
    delay(wait);
  }

  strip.show(); // Refresh to turn off last pixel
}

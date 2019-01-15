/*
	RealtimeAudioLEDsMain.ino
	Written by Ryan Davis on 4/28/18 for Teensy 3.6
*/
	
#include <OctoWS2811.h>
#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>

/*
	Define NUM_LEDS_PER_STRIP as the strip with the most LEDs
	OctoWS2811 Library uses pins 2 and 14  of the Teensy 3.6 for the first two LED strips by default 
	The 8x20 Matrix on the front of the hoodie is 160 LEDs total wired as one strip, connected to pin 2
	The strip wrapped around the arms of the hoodie is 140 LEDs, connected to pin 14
*/
#define NUM_LEDS_PER_STRIP 160
#define NUM_STRIPS 8

uint32_t colors[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xFF1088, 0xE05800, 0xFFFFFF};

long millisLast = 0;

// pick new random color
int newColor = colors[random(0,6)];

// seperate new random color into red, green, and blue values for WS2811 LED Operation
int redVal = (newColor & 0xFF0000) >> 16;
int greenVal = (newColor & 0x00FF00) >> 8;
int blueVal = newColor & 0x0000FF;

// The display size and color to use
const unsigned int matrix_width = 20;
const unsigned int matrix_height = 8;

// These parameters adjust the vertical thresholds of the matrix
const float maxLevel = 0.3;      // 1.0 = max, lower is more "sensitive"
const float dynamicRange = 30.0; // total range to display, in decibels
const float linearBlend = 0.7;   // useful range is 0 to 0.7

// OctoWS2811 objects
const int ledsPerPin = matrix_width * matrix_height;
DMAMEM int displayMemory[ledsPerPin*6];
int drawingMemory[ledsPerPin*6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerPin, displayMemory, drawingMemory, config);

// Audio library objects
// define A2 as input from audio jack
AudioInputAnalog         adc1(A2);
// create FFT object
AudioAnalyzeFFT1024     fft;
// create peak analyzer object
AudioAnalyzePeak        peak;
// connect audio input to FFT and peak analyzer object
AudioConnection          patchCord1(adc1, fft);
AudioConnection          patchCord2(adc1, peak);




/*
	This array holds the volume level (0 to 1.0) for each
	vertical pixel to turn on.  Computed in setup() using
	the 3 parameters above.
*/
float thresholdVertical[matrix_height];

/* 
	This array specifies how many of the FFT frequency bin
	to use for each horizontal pixel.  Because humans hear
	in octaves and FFT bins are linear, the low frequencies
	use a small number of bins, higher frequencies use more.
*/
int frequencyBinsHorizontal[matrix_width] = {
   1,  1,  1,  1,
   2,  2,  2,  3,  4,  4,
   6,  7,  8,  9, 11, 13,
  15, 18, 22, 25
};

// Run setup once
void setup() {
  // the audio library needs to be given memory to start working
  AudioMemory(12);
  fft.windowFunction(AudioWindowHanning1024);
  // compute the vertical thresholds before starting
  computeVerticalLevels();

  Serial.begin(9600);

  // turn on the display
  leds.begin();
  leds.show();
}

// Run once from setup, the compute the vertical levels
void computeVerticalLevels() {
  unsigned int y;
  float n, logLevel, linearLevel;

  for (y=0; y < matrix_height; y++) {
    n = (float)y / (float)(matrix_height - 1);
    logLevel = pow10f(n * -1.0 * (dynamicRange / 20.0));
    linearLevel = 1.0 - n;
    linearLevel = linearLevel * linearBlend;
    logLevel = logLevel * (1.0 - linearBlend);
    thresholdVertical[y] = (logLevel + linearLevel) * maxLevel;
  }
}

// A simple xy() function to turn display matrix coordinates
// into the index numbers OctoWS2811 requires. 
unsigned int xy(unsigned int x, unsigned int y) {
y = 7-y;
if ((y & 1) == 0) {
    // even numbered rows (0, 2, 4...) are left to right
    return y * matrix_width + x;
  } else {
    // odd numbered rows (1, 3, 5...) are right to left
    return y * matrix_width + matrix_width - 1 - x;
  }
}

// Run repetitively
void loop() {
  // if 20 seconds have passed, select new random color
  if (millisLast+20000 < millis()){
    int newColor = colors[random(0,6)];
    redVal = (newColor & 0xFF0000) >> 16;
    greenVal = (newColor & 0x00FF00) >> 8;
    blueVal = newColor & 0x0000FF;
    millisLast = millis();
  }
  
  unsigned int x, y, freqBin;
  float level;

  if (fft.available()) {
    // freqBin counts which FFT frequency data has been used,
    // starting at low frequency
    freqBin = 0;

    for (x=0; x < matrix_width; x++) {
      // get the volume for each horizontal pixel position
      level = fft.read(freqBin, freqBin + frequencyBinsHorizontal[x] - 1);

      for (y=0; y < matrix_height; y++) {
        // for each vertical pixel, check if above the threshold
        // and turn the LED on or off
        if (level >= thresholdVertical[y]) {
          leds.setPixel(xy(x, y), redVal, greenVal, blueVal);
        } else {
          leds.setPixel(xy(x, y), 0x000000);
        }
      }
      // increment the frequency bin count, so we display
      // low to higher frequency from left to right
      freqBin = freqBin + frequencyBinsHorizontal[x];
    }

	// if peak measure available, set the brightness of the hoodie's
	// arm LEDs to a scalar value of the peak level
      if (peak.available()) {
        float peakMeasure = fft.read(1, 155)/15;
        Serial.println(peakMeasure);
        for (int stripLed = 161; stripLed < 300; stripLed++) {
          leds.setPixel(stripLed, redVal*peakMeasure, greenVal*peakMeasure, blueVal*peakMeasure);
        }
      }
    
    // after all pixels set, show them all at the same instant
    leds.show();
  }

}
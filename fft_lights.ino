/*
  Copyright (c) 2019 Shajeeb TM

  Permission is hereby granted,  free of charge, to any person obtaining a copy
  of this software and associated  documentation files (the "Software"), to deal
  in the Software without restriction,  including without limitation the rights
  to use, copy, modify, merge, publish,  distribute, sublicense, and/or sell
  copies of the Software, and to permit persons  to whom the Software is
  furnished to do so, subject to the following conditions:
  The  above copyright notice and this permission notice shall be included in all
  copies  or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS",  WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO  THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES  OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING  FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS  IN THE
  SOFTWARE.
*/

#include <arduinoFFT.h>

#include <PWM.h>

#define SAMPLES 32           //Must be a power of 2
#define  xres 4   // Total number of  columns in  the display, must be <= SAMPLES/2
#define  yres 255      // Total number of  rows  in the display

double vReal[SAMPLES];
double vImag[SAMPLES];
char data_avgs[xres];

int  yvalue;
int peaks[xres];
int red = 11;
int green = 10;
int blue = 9;
int frequency = 1000;

int MY_SPECIAL_ARRAY[] = {0, 16, 32, 64, 96, 128, 160, 192, 224, 255};

arduinoFFT  FFT = arduinoFFT();                                    // FFT object

void  setup() {
  InitTimersSafe();
  Serial.begin(115200);
  ADCSRA = 0b11100101;      // set ADC to free running mode  and set pre-scalar to 32 (0xe5)
  ADMUX = 0b00000000;       // use pin A0 and  external voltage reference
  //pinMode(buttonPin, INPUT);
  OSCCAL = 240;
  bool success = SetPinFrequencySafe(red, frequency);
  bool success = SetPinFrequencySafe(blue, frequency);
  bool success = SetPinFrequencySafe(green, frequency);
  delay(50);            // wait to get reference  voltage stabilized
}

void loop() {
  // ++ Sampling
  for (int  i = 0; i < SAMPLES; i++)
  {
    while (!(ADCSRA & 0x10));       // wait for  ADC to complete current conversion ie ADIF bit set
    ADCSRA = 0b11110101  ;               // clear ADIF bit so that ADC can do next operation (0xf5)
    int  value = ADC - 512 ;                 // Read from ADC and subtract DC offset caused  value
    //Serial.println(ADC);
    vReal[i] = value / 8;                   // Copy to bins after compressing
    vImag[i] = 0;
  }
  // -- Sampling


  // ++ FFT
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal,  vImag, SAMPLES);
  // -- FFT


  // ++ re-arrange FFT result  to match with no. of columns on display ( xres )
  int step = (SAMPLES / 2) / xres;
  int c = 0;
  for (int i = 0; i < (SAMPLES / 2); i += step)
  {
    data_avgs[c]  = 0;
    for (int k = 0 ; k < step ; k++) {
      data_avgs[c] = data_avgs[c]  + vReal[i + k];
    }
    data_avgs[c] = data_avgs[c] / step;
    c++;
  }
  // -- re-arrange FFT result to match with no. of columns on display  ( xres )


  // ++ send to display according measured value
  for (int i = 0; i < xres; i++)
  {
    data_avgs[i] = constrain(data_avgs[i], 0, 80);          // set max & min values for buckets
    data_avgs[i] = map(data_avgs[i],  0, 80, 0, yres);        // remap averaged values to yres
    yvalue = data_avgs[i];
    /*
        peaks[i] = peaks[i] - 1;  // decay by one light
        if (yvalue > peaks[i]) {
          peaks[i] = yvalue ;
        }
        //yvalue = yvalue - 255;
    */
    int bright = yvalue;
    //MY_SPECIAL_ARRAY[yvalue];

    switch (i) {
      case 0 :
        pwmWrite(red, bright);
        break;
      case 1 :
        pwmWrite(green, bright);
        break;
      case 2 :
        pwmWrite(blue, bright);
        break;
    }
  }
}

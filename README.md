# RealtimeAudioHoodie
Code for a hoodie that renders an audio input on 300 WS2811 LEDs in realtime.
Features a 3.5mm audio input, an 8x20 matrix of 160 LEDs mounted on the chest, and a strip of 140 LEDs wrapped around the arms.

## Operating Process:
The audio input is converted to a digital signal by a Teensy 3.6, which runs a Fast Fourier Transform and a peak measure.
The FFT results are used to calculate values to display a bar graph on the matrix, and the peak measure is used to scale the brightness of the arm strip.
Finally, the Teensy updates the LEDs using Direct Memory Access provided by the OctoWS2811 library, allowing minimal update overhead and allowing the devotion of the entire processor to FFT/peak calculation. Using this technique, the LEDs can update at a framerate surpassing 300 frames per second.
More details are given in the code file, which requires only default Teensy libraries to run.

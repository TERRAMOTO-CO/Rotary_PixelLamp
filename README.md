///////// Breakdown of the lamp to help code readability /////////

INTERACTION FLOW
  - User plays with potentiometers to selected a row and a column
      - Row and Column both begin pulsing at first state change detection to isolate a single pixel at their intersection. This pixel is left solid to aid in visual selection
      - A timeout function ceases the pulsing animation of row and column simultaneuosly after 30 seconds from last deteced change
  - User dials six times to create a string of values that represent a HEX color value using the function button as required for alpha charatcers
  - After 6 inputs are dialed, Arduino coverts HEX string to RGBW values and assigns specific value to the selected pixel in the matrix array
  - Interactions continue...
  
/// HARDWARE OVERVIEW ///
OUTPUTS:
A lamp that uses a 6x10 matrix of LEDs (RGBWW WS2812 Neopixels) controlled by an Arduino Uno
  - Matrix is wired vertically column to column

INPUTS:    
Lamp is programed by user via a removable controller that has 2 potetiometers, a momentary NO push button, and a rotary dial input
  - 10ohm Potentiometers control column and row selection
  - Normally Closed Momentary Push Button changes the type of input from the rotary phone input from 0-9 to A-F (and a clear function mapped to Fn0)
  - Rotary Phone input has two signal inputs
      - "is rotating" signal is a normally closed switch that opens while the dial is spinning back home
      - "pulse count" signal assigns the selected input value (1 Pulse = 1; 2 pulses = 2; ... 10 pulses = 0; Fn+1 pulse = A; ... Fn+6 pulses = F)


KNOWN ISSUES:
-- memoryClearCount Function is broken.
    - Concept attempts to add double function to the memory clear. 
      - A single count wipes the current dialed string so users can redial 6 values
      - A double count (should but doesn't) wipe the whole matrix back to its default color values

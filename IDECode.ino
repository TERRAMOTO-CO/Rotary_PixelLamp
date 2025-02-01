#include <Adafruit_NeoPixel.h>
#include <math.h>

#define NUM_LEDS 60                // Total number of LEDs in the 6x10 matrix
#define DATA_PIN 6                 // Data pin for the NeoPixel strip
#define POT_COLUMN A0              // Potentiometer for selecting column
#define POT_ROW A1                 // Potentiometer for selecting row
#define BUTTON_PIN 10              // Momentary push button for A-F mode
#define INIT_PIN 12                // Initial pin for detecting dial turning
#define NUM_PIN 11                 // Pulse pin for counting dial pulses
#define DEBOUNCE_DELAY 5           // Debounce delay for rotary dial pulses
#define DEBOUNCE_TIME 50           // Button debounce time in ms
#define TIMEOUT 60000              // 1-minute timeout for fading
#define MIN_BRIGHTNESS 0.5         // Minimum brightness multiplier
#define MAX_BRIGHTNESS 1.5         // Maximum brightness multiplier

Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_GRBW + NEO_KHZ800);

// Global variables for rotary dial debounce and state tracking
unsigned long lastDebounceTime = 0;  // Track the last debounce time
int currentValue = HIGH;            // Track the current pulse pin state
unsigned long lastChangeTime = 0;  // Last time potentiometers were adjusted
unsigned long lastFadeTime = 0;    // Last time LEDs were updated in fade effect
unsigned long lastButtonPress = 0; // Last button press time
bool isFading = false;             // Flag to indicate fading is active
bool buttonPressed = false;        // Track button state

int selectedColumn = -1;           // Current column (-1 means none)
int selectedRow = -1;              // Current row (-1 means none)

int prevColumnValue = 0;  // Store previous column potentiometer value
int prevRowValue = 0;     // Store previous row potentiometer value

float brightness = 1.0;            // Brightness multiplier
float fadeAmount = 0.05;           // Brightness increment/decrement

uint32_t warmWhite = strip.Color(70, 42, 10, 70);  // Warm white color
uint32_t storedColors[NUM_LEDS];                   // Store LED colors

int counter = 0;                   // Rotary dial pulse counter
int memoryClearCount = 0;          // Memory clear count
int hexValues[6];                  // Store hex values
int index = 0;                     // Hex value index
bool alphaMode = false;            // Toggle for A-F input mode

bool remoteConnected = true; // Tracks if the remote is connected
unsigned long remoteCheckInterval = 1000; // Interval to check for remote connection (in ms)
unsigned long lastRemoteCheckTime = 0;    // Last time the remote was checked


void setup() {
    Serial.begin(9600);
  Serial.println("Starting setup...");
  strip.begin();
  strip.show();

  // Initialize all LEDs to warm white
  for (int i = 0; i < NUM_LEDS; i++) {
    storedColors[i] = warmWhite;
    strip.setPixelColor(i, warmWhite);
  }
  strip.show();


  pinMode(INIT_PIN, INPUT_PULLUP);
  pinMode(NUM_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("Setup complete. Ready to monitor inputs.");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Periodically check the remote connection
  if (currentTime - lastRemoteCheckTime >= remoteCheckInterval) {
    checkRemoteConnection();
    lastRemoteCheckTime = currentTime;
  }

  // Only run input-related functions if the remote is connected
  if (remoteConnected) {
    checkPotentiometers();
    fadeSelectedRowColumn();
    handleRotaryDial();
    checkButton();
  } else {
    Serial.println("Remote disconnected: Inputs are disabled.");
  }

  checkTimeout();
}


// Map row/column to the correct LED index in zigzag wiring pattern
int getLEDIndex(int column, int row) {
  if (column % 2 == 0) {
    return column * 10 + row;       // Even column (bottom to top)
  } else {
    return column * 10 + (9 - row); // Odd column (top to bottom)
  }
}
// Check potentiometer values with software debounce
void checkPotentiometers() {
  int rawColumnValue = analogRead(POT_COLUMN);
  int rawRowValue = analogRead(POT_ROW);

  // Apply debounce: only update if the difference exceeds a threshold
  if (abs(rawColumnValue - prevColumnValue) > 10) {
    prevColumnValue = rawColumnValue;
    int newColumn = map(prevColumnValue, 20, 1000, 0, 5);
    if (newColumn != selectedColumn) {
   selectedColumn = newColumn;
   isFading = true;  // Enable fading
   lastChangeTime = millis();  // Reset timeout
   Serial.print("Selected Column: ");
   Serial.println(selectedColumn);
}

  }

  if (abs(rawRowValue - prevRowValue) > 10) {
    prevRowValue = rawRowValue;
    int newRow = map(prevRowValue, 50, 1000, 0, 9);
    if (newRow != selectedRow) {
   selectedRow = newRow;
   isFading = true;  // Enable fading
   lastChangeTime = millis();  // Reset timeout
   Serial.print("Selected Row: ");
   Serial.println(selectedRow);
}

  }
}

// Fade effect for selected row and column
void fadeSelectedRowColumn() {
  if (isFading && millis() - lastFadeTime >= 30) {  // Update every 30 ms
    lastFadeTime = millis();

    // Update brightness multiplier
    brightness += fadeAmount;

    // Reverse direction at brightness limits
    if (brightness <= MIN_BRIGHTNESS || brightness >= MAX_BRIGHTNESS) {
      fadeAmount = -fadeAmount;
    }

    // Apply fade effect to the selected column
    for (int row = 0; row < 10; row++) {
      int ledIndex = getLEDIndex(selectedColumn, row);
      if (row != selectedRow) {
        applyFade(ledIndex);
      }
    }

    // Apply fade effect to the selected row
    for (int col = 0; col < 6; col++) {
      int ledIndex = getLEDIndex(col, selectedRow);
      if (col != selectedColumn) {
        applyFade(ledIndex);
      }
    }

    strip.show();
  }
}

// Apply fading effect to an LED based on stored color
void applyFade(int ledIndex) {
  uint32_t baseColor = storedColors[ledIndex];
  uint8_t r = (baseColor >> 16) & 0xFF;  // Red component
  uint8_t g = (baseColor >> 8) & 0xFF;   // Green component
  uint8_t b = baseColor & 0xFF;          // Blue component
  uint8_t w = (baseColor >> 24) & 0xFF;  // White component

  // Scale each component by the brightness multiplier
  r = constrain(r * brightness, 0, 255);
  g = constrain(g * brightness, 0, 255);
  b = constrain(b * brightness, 0, 255);
  w = constrain(w * brightness, 0, 255);

  strip.setPixelColor(ledIndex, strip.Color(r, g, b, w));
}

// Restore stored colors to all LEDs
void restoreStoredColors() {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, storedColors[i]);
  }
  strip.show();
}

// Timeout for fading effect
void checkTimeout() {
  if (isFading && (millis() - lastChangeTime > TIMEOUT)) {
    isFading = false;  // Stop fading
    restoreStoredColors();
    Serial.println("Fading stopped due to timeout.");
  }
}
// Handles rotary dial input
void handleRotaryDial() {
  int initRead = digitalRead(INIT_PIN); // Check if the dial is turning
  static int lastValue = HIGH;         // Holds the last read from the pulse pin
  static unsigned long lastPulseTime = 0;  // Time of the last pulse

  if (initRead == LOW) { // If the wheel is turning...
    int newValue = digitalRead(NUM_PIN); // Check the pulse pin

    // Debounce logic
    if (newValue != lastValue && (millis() - lastPulseTime) > DEBOUNCE_DELAY) {
      lastPulseTime = millis();  // Update the last pulse time
      if (newValue == LOW) {     // If the signal drops to LOW
        counter++;               // Increment the counter
        Serial.print("Pulse detected. Count: ");
        Serial.println(counter);
      }
    }
    lastValue = newValue; // Update the last value for comparison
  } else if (counter > 0) { // If the dial returns home
    Serial.print("Dial complete. Final count: ");
    Serial.println(counter);

    if (buttonPressed) {
      // Alphabetic mode (A-F)
      if (counter == 10) {
        Serial.println("Memory Cleared");
        index = 0; // Clear the stored hex values
      } else if (counter <= 6) {
        hexValues[index++] = 10 + (counter - 1); // Store A-F as 10-15
        Serial.print("Dialed Letter: ");
        Serial.println((char)('A' + (counter - 1)));
      }
    } else {
      // Numeric mode (0-9)
      hexValues[index++] = (counter == 10) ? 0 : counter; // Store 0-9
      Serial.print("Dialed Number: ");
      Serial.println(hexValues[index - 1]);
    }

    if (index >= 6) { // If all 6 values are entered
      int red = (hexValues[0] * 16) + hexValues[1];
      int green = (hexValues[2] * 16) + hexValues[3];
      int blue = (hexValues[4] * 16) + hexValues[5];
      int ledIndex = getLEDIndex(selectedColumn, selectedRow);

      Serial.print("Hex Code: #");
      for (int i = 0; i < 6; i++) {
        Serial.print(hexValues[i], HEX);
      }
      Serial.print(" -> RGB: (");
      Serial.print(red);
      Serial.print(", ");
      Serial.print(green);
      Serial.print(", ");
      Serial.print(blue);
      Serial.println(")");

      setColor(ledIndex, red, green, blue, 0);  // Update the selected LED
      index = 0; // Reset for next input
    }

    counter = 0; // Reset counter
  }
}
void checkRemoteConnection() {
  static int prevColumnValue = analogRead(POT_COLUMN);
  static int prevRowValue = analogRead(POT_ROW);

  int currentColumnValue = analogRead(POT_COLUMN);
  int currentRowValue = analogRead(POT_ROW);

  // Calculate the difference between consecutive readings
  int columnDifference = abs(currentColumnValue - prevColumnValue);
  int rowDifference = abs(currentRowValue - prevRowValue);

  // Determine if the input is unstable or unconnected
  if (columnDifference > 50 || rowDifference > 50) { // Threshold for unconnected behavior
    remoteConnected = false;
    Serial.println("Remote disconnected. Disabling input functions.");
  } else {
    remoteConnected = true;
    Serial.println("Remote connected. Enabling input functions.");
  }

  // Update the previous values for the next check
  prevColumnValue = currentColumnValue;
  prevRowValue = currentRowValue;
}


// Process rotary dial input
void processDialInput() {
  if (alphaMode) {
    if (counter <= 6) {
      hexValues[index++] = 10 + (counter - 1);  // A-F
      Serial.print("Dialed Letter: ");
      Serial.println((char)('A' + (counter - 1)));
    }
  } else {
    hexValues[index++] = (counter == 10) ? 0 : counter;  // 0-9
    Serial.print("Dialed Number: ");
    Serial.println(hexValues[index - 1]);
  }

  if (index >= 6) {
    applyHexToSelectedLED();
  }
}

// Apply hex color to selected LED
void applyHexToSelectedLED() {
  int red = (hexValues[0] * 16) + hexValues[1];
  int green = (hexValues[2] * 16) + hexValues[3];
  int blue = (hexValues[4] * 16) + hexValues[5];
  int ledIndex = getLEDIndex(selectedColumn, selectedRow);

  setColor(ledIndex, red, green, blue, 0);
  index = 0;
  memoryClearCount = 0;
}

// Set color for an individual LED
void setColor(int ledIndex, int red, int green, int blue, int white) {
  uint32_t color = strip.Color(red, green, blue, white);
  storedColors[ledIndex] = color;
  strip.setPixelColor(ledIndex, color);
  strip.show();
}

// Handles memory clearing logic
void handleMemoryClear() {
    static unsigned long lastClearTime = 0;  // Store the time of the last memory clear action
    static unsigned long countdownStart = 0; // For countdown debugging
    unsigned long currentTime = millis();   // Get the current time

    // Check for timeout and reset the counter if needed
    if (currentTime - lastClearTime > 3000) {
        if (memoryClearCount > 0) { // Only print if the counter was reset
            Serial.println("3 seconds passed. Memory clear counter reset to 0.");
        }
        memoryClearCount = 0;
        countdownStart = 0; // Stop countdown tracking
    }

    // If a new action happens, reset the countdown timer
    if (memoryClearCount == 0) {
        countdownStart = currentTime;
    }

    // Increment memory clear count
    memoryClearCount++;
    lastClearTime = currentTime; // Update the last clear time

    // Debugging: Show the current state of memoryClearCount
    Serial.print("Memory Clear Count: ");
    Serial.println(memoryClearCount);

    // Countdown debug
    if (memoryClearCount == 1) {
        Serial.println("Countdown started: You have 3 seconds to press again for a full reset.");
    }

    if (memoryClearCount == 1 && countdownStart > 0) {
        for (int i = 3; i > 0; i--) { // Countdown from 3
            unsigned long timeLeft = 3000 - (millis() - countdownStart);
            if (timeLeft > (i - 1) * 1000 && timeLeft <= i * 1000) {
                Serial.print("Time remaining: ");
                Serial.print(i);
                Serial.println(" seconds.");
            }
        }
    }

    // Action for single memory clear
    if (memoryClearCount == 1) {
        Serial.println("Memory Cleared: Hex values reset.");
        index = 0;  // Reset hex input
    }

    // Action for full reset
    if (memoryClearCount == 2) {
        Serial.println("Full Reset: All LEDs set to default warm white.");

        // Reset all LEDs to warm white
        for (int i = 0; i < NUM_LEDS; i++) {
            storedColors[i] = warmWhite;  // Update the stored array
            strip.setPixelColor(i, warmWhite);  // Update the LED
            Serial.print("Resetting LED ");
            Serial.println(i);
        }

        strip.show();  // Refresh the LED strip
        Serial.println("LED reset complete.");

        memoryClearCount = 0;  // Reset the counter after a full reset
        countdownStart = 0;    // Stop countdown tracking
    }
}




// Check button for A-F toggle with debounce
void checkButton() {
  int buttonReading = digitalRead(BUTTON_PIN);

  if (buttonReading == LOW && !buttonPressed) {
    unsigned long currentTime = millis();
    if (currentTime - lastButtonPress > DEBOUNCE_TIME) {
      buttonPressed = true;
      alphaMode = !alphaMode;
      Serial.println("Button Pressed: Mode toggled.");
      lastButtonPress = currentTime;
    }
  }

  if (buttonReading == HIGH && buttonPressed) {
    buttonPressed = false;
  }
}

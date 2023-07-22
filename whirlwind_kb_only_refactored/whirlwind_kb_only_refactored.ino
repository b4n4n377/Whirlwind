/**
 * @file whirlwind_kb_only_refactored.cpp
 * @author Banane (from GroovyArcade Discord)
 * @date 22 July 2023
 * @brief This is a RP2040 firmware for the open source PC to Arcade Cabinet Interface
 * designed by Barito Nomarchetto.
 *
 * It is supposed to be an improvement of the initial version
 * https://github.com/baritonomarchetto/Whirlwind/blob/main/whirlwind.ino
 *
 * The complete project can be found here
 * https://www.instructables.com/Whirlwind-PC-to-JAMMA-Arcade-Cabinet-Interface/
 *
 * This program reads button states from an arcade controller and simulates keyboard
 * presses accordingly. It also provides optional frequency blocking functionality.
 *
 * Dependencies: This program depends on the Keyboard and pico/stdlib libraries.
 * It is designed to work with the RP2040 microcontroller.
 *
 * Button and shift functionality are handled separately. Each button can have a normal
 * and a shifted function, much like a real keyboard.
 *
 * Button debouncing is implemented to avoid multiple inputs from a single press.
 *
 * When SYNC_MONITOR_ACTIVE is defined, the frequency block functionality is enabled.
 * This checks the frequency of the horizontal sync pin and enables or disables
 * the video amp and LED accordingly.
 */
#include <Keyboard.h> // Keyboard library
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define SW_INPUTS 26 // Switch inputs

#define SYNC_MONITOR_ACTIVE // Activates sync check if defined
#define HSyncPin 0
#define disablePin 1
#define ledPin 25
#define KEY_SPACE 0x20

#define DEBOUNCE_TIME 10 // Debounce time in milliseconds
#define PULSE_COUNT 100

unsigned long millisNow = 0;
uint32_t buttonStates = 0;

struct digitalInput
{
  const byte pin;
  unsigned long debTimestamp; // Timestamp for debouncing
  boolean state;
  const uint8_t btn;
  const uint8_t btn_shift;
};

digitalInput inputs[SW_INPUTS] = {
    // Define buttons: {Pin, debounce timestamp, state, button, shift button}
    // Check:
    // https://www.arduino.cc/reference/en/language/functions/usb/keyboard/keyboardmodifiers/
    // https://docs.mamedev.org/usingmame/defaultkeys.html

    {6, 0, HIGH, '1', '1'},                          // P1 START - SHIFT BUTTON
    {7, 0, HIGH, '2', KEY_ESC},                      // P2 START, ESC if shifted
    {8, 0, HIGH, KEY_UP_ARROW, KEY_UP_ARROW},        // P1 UP
    {10, 0, HIGH, KEY_DOWN_ARROW, KEY_DOWN_ARROW},   // P1 DOWN
    {16, 0, HIGH, KEY_LEFT_ARROW, KEY_LEFT_ARROW},   // P1 LEFT
    {14, 0, HIGH, KEY_RIGHT_ARROW, KEY_RIGHT_ARROW}, // P1 RIGHT
    {12, 0, HIGH, KEY_LEFT_CTRL, KEY_TAB},           // P1 B1, TAB if shifted
    {20, 0, HIGH, KEY_LEFT_ALT, KEY_RETURN},         // P1 B2, ENTER if shifted
    {18, 0, HIGH, KEY_SPACE, KEY_F11},               // P1 B3, F11 if shifted
    {22, 0, HIGH, KEY_LEFT_SHIFT, KEY_LEFT_SHIFT},   // P1 B4
    {24, 0, HIGH, 'z', 'z'},                         // P1 B5
    {27, 0, HIGH, 'x', 'x'},                         // P1 B6
    {9, 0, HIGH, 'r', 'r'},                          // P2 UP
    {11, 0, HIGH, 'f', 'f'},                         // P2 DOWN
    {17, 0, HIGH, 'd', 'd'},                         // P2 LEFT
    {15, 0, HIGH, 'g', 'g'},                         // P2 RIGHT
    {13, 0, HIGH, 'a', 'a'},                         // P2 B1
    {21, 0, HIGH, 's', 's'},                         // P2 B2
    {19, 0, HIGH, 'q', 'q'},                         // P2 B3
    {23, 0, HIGH, 'w', 'w'},                         // P2 B4
    {26, 0, HIGH, 'e', 'e'},                         // P2 B5
    {28, 0, HIGH, 'y', 'y'},                         // P2 B6
    {4, 0, HIGH, '5', '6'},                          // P1 COIN
    {5, 0, HIGH, '6', '6'},                          // P2 COIN
    {3, 0, HIGH, KEY_F2, KEY_F2},                    // SERVICE SW
    {2, 0, HIGH, '9', '9'},                          // TEST SW
};

boolean startBlock = false; // Block shift key

/**
 * @brief Main setup function. Initializes the digital inputs, sets pin modes and starts the Keyboard library
 *
 * @details The function first initializes all the button pins as inputs with pull-up resistors
 * and sets their initial state. Then, the horizontal sync pin, the LED pin and disable pin are set up.
 * In the end, the Keyboard library is initialized.
 */
void setup()
{

  // Set all the button pints to inputs with pull-up resistors and set their initial state to HIGH
  for (int i = 0; i < SW_INPUTS; i++)
  {
    pinMode(inputs[i].pin, INPUT_PULLUP);         // Set input pin
    inputs[i].state = digitalRead(inputs[i].pin); // Read state
  }

  // Set HSyncPin as input to measure the frequency
  pinMode(HSyncPin, INPUT);

  // Set LED-Pin as output (as LED switch) and it's initial state
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Set disable-Pin as output (as video amp switch) and it's initial state
  pinMode(disablePin, OUTPUT);
  digitalWrite(disablePin, HIGH);

  // Init Keyboard library
  Keyboard.begin();
}

/**
 * @brief Main program loop that handles buttons and optionally checks frequency block
 *
 * @details The function first calls the handleShiftButton() function to process the shift button,
 * then the handleButtons() function to process all other buttons. If the SYNC_MONITOR_ACTIVE macro is defined,
 * it also calls the freqBlock() function to check the frequency block status.
 */
void loop()
{

  handleShiftButton(); // Process the shift button separately
  handleButtons();     // Process the rest of the buttons
#ifdef SYNC_MONITOR_ACTIVE
  freqBlock(); // Check if the frequency block is active
#endif
}

/**
 * @brief Function that handles button states except for the shift button
 *
 * @details This function reads and processes the state of all buttons except for shift.
 * It implements button state reading, debouncing, and key pressing. Depending on the state
 * of each button and the shift button, it either presses or releases keys on the keyboard.
 */
void handleButtons()
{

  // Get current time since start
  millisNow = millis();

  // Read the Button states via RP2040's GPIO hardware registers
  uint32_t gpioStates = gpio_get_all();

  // Set up the button states with bitwise operations
  for (int i = 1; i < SW_INPUTS; i++)
  {
    // Isolate the state of this button
    uint32_t buttonState = (gpioStates & (1UL << inputs[i].pin)) != 0;
    uint32_t shiftState = inputs[0].state;

    // If the state has changed and it's been long enough since the last change (debouncing)
    if (inputs[i].state != buttonState && millisNow - inputs[i].debTimestamp >= DEBOUNCE_TIME)
    {
      // Update the state and debounce timestamp55555555555555555
      inputs[i].state = buttonState;
      inputs[i].debTimestamp = millisNow;

      if (shiftState == HIGH && buttonState == LOW)
      {                                // no shift + button pressed
        Keyboard.press(inputs[i].btn); // press key
      }

      if (shiftState == HIGH && buttonState == HIGH)
      {                                  // no shift + button released
        Keyboard.release(inputs[i].btn); // release key
      }

      if (shiftState == LOW && buttonState == LOW)
      {                                      // shift + button pressed
        startBlock = true;                   // block shift
        Keyboard.press(inputs[i].btn_shift); // press shifted key
      }

      if (shiftState == LOW && buttonState == HIGH)
      {                                        // shift + button released
        Keyboard.release(inputs[i].btn_shift); // press shifted key
      }
    }
  }
}

/**
 * @brief Function that handles the shift button separately
 *
 * @details This function reads and processes the state of the shift button.
 * It implements shift state reading, debouncing, and key pressing. Depending on the state
 * of the shift button, it either presses and releases the key or unblocks the shift key.
 */
void handleShiftButton()
{

  // Get current time since start
  millisNow = millis();

  // Read the Button states via RP2040's GPIO hardware registers
  uint32_t gpioStates = gpio_get_all();

  // Set up the button state with bitwise operations
  uint32_t buttonState = (gpioStates & (1UL << inputs[0].pin)) != 0;

  // If the state has changed and it's been long enough since the last change (debouncing)
  if (inputs[0].state != buttonState && millisNow - inputs[0].debTimestamp >= DEBOUNCE_TIME)
  {
    // Update the state and debounce timestamp
    inputs[0].state = buttonState;
    inputs[0].debTimestamp = millisNow;

    uint32_t shiftState = inputs[0].state;

    if (startBlock == false && shiftState == HIGH)
    {                                  // shift unblocked + released
      Keyboard.press(inputs[0].btn);   // press key
      Keyboard.release(inputs[0].btn); // press key
    }

    if (startBlock == true && shiftState == HIGH)
    {                     // shift blocked + released
      startBlock = false; // reset block state
    }
  }
}

/**
 * @brief Function that handles sync frequency and switches the video amp and LED
 *
 * @details This function measures the pulse frequency of the horizontal sync pin
 * and enables the video amp and switches on the LED if the frequency is within a specified range.
 * If no pulses are detected, it switches off the LED and disables the video amp.
 */
void freqBlock()
{

  unsigned long totalDuration = 0; // Variable to store the total duration of pulses
  bool hasNoPulse = false;         // Flag to indicate if pulses are detected

  for (int pulseCount = 0; pulseCount < PULSE_COUNT; pulseCount++)
  {
    unsigned long pulseDuration = pulseIn(HSyncPin, HIGH); // Measure the pulse duration
    totalDuration += pulseDuration;                        // Accumulate the pulse duration

    if (pulseDuration == 0)
    {
      hasNoPulse = true;
      break; // Exit the loop immediately if no pulses are detected
    }
  }

  if (!hasNoPulse)
  {
    float averageDuration = totalDuration / 100.0;                                   // Calculate the average pulse duration
    float frequency = 100000.0 / totalDuration;                                      // Calculate the frequency in kHz
    digitalWrite(ledPin, (frequency >= 12.0 && frequency <= 18.0) ? HIGH : LOW);     // Switch the LED based on frequency range
    digitalWrite(disablePin, (frequency >= 12.0 && frequency <= 18.0) ? LOW : HIGH); // Switch video amp based on freuency range
  }
  else
  {
    digitalWrite(ledPin, LOW);      // Switch off the LED
    digitalWrite(disablePin, HIGH); // Set disablePin to HIGH and turn off video amp
  }
}

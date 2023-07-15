#include <Keyboard.h> // Keyboard library

#define SW_INPUTS 26 // Switch inputs
#define HSyncPin 0
#define disablePin 1
#define ledPin 25
#define debTime 40      // Debounce time (ms)
#define samples 100     // Frequency block samples
#define DEFAULT_FREQ 55 // Default sync freq
// #define SYNC_MONITOR_ACTIVE // Activates sync check if defined

struct digitalInput
{
  const byte pin;
  boolean state;
  unsigned long dbTime;
  const uint8_t btn;
  const uint8_t btn_shift;
}

digitalInput inputs[SW_INPUTS] = {
    // Define buttons: {Pin, state, debounce time, button, shift button}
    // Check:
    // https://www.arduino.cc/reference/en/language/functions/usb/keyboard/keyboardmodifiers/

    {4, HIGH, 0, '5', '5'},                          // P1 COIN
    {5, HIGH, 0, '6', '6'},                          // P2 COIN
    {3, HIGH, 0, KEY_F2, KEY_F2},                    // SERVICE SW
    {2, HIGH, 0, '9', '9'},                          // TEST SW
    {6, HIGH, 0, '1', '1'},                          // P1 START - SHIFT BUTTON
    {7, HIGH, 0, '2', '2'},                          // P2 START
    {8, HIGH, 0, KEY_UP_ARROW, KEY_UP_ARROW},        // P1 UP
    {10, HIGH, 0, KEY_DOWN_ARROW, KEY_DOWN_ARROW},   // P1 DOWN
    {16, HIGH, 0, KEY_LEFT_ARROW, KEY_LEFT_ARROW},   // P1 LEFT
    {14, HIGH, 0, KEY_RIGHT_ARROW, KEY_RIGHT_ARROW}, // P1 RIGHT
    {12, HIGH, 0, KEY_LEFT_CTRL, KEY_LEFT_CTRL},     // P1 B1
    {20, HIGH, 0, KEY_LEFT_ALT, KEY_LEFT_ALT},       // P1 B2
    {18, HIGH, 0, ' ', ' '},                         // P1 B3
    {22, HIGH, 0, KEY_LEFT_SHIFT, KEY_LEFT_SHIFT},   // P1 B4
    {24, HIGH, 0, 'z', 'z'},                         // P1 B5
    {27, HIGH, 0, 'x', 'x'},                         // P1 B6
    {9, HIGH, 0, 'r', 'r'},                          // P2 UP
    {11, HIGH, 0, 'f', 'f'},                         // P2 DOWN
    {17, HIGH, 0, 'd', 'd'},                         // P2 LEFT
    {15, HIGH, 0, 'g', 'g'},                         // P2 RIGHT
    {13, HIGH, 0, 'a', 'a'},                         // P2 B1
    {21, HIGH, 0, 's', 's'},                         // P2 B2
    {19, HIGH, 0, 'q', 'q'},                         // P2 B3
    {23, HIGH, 0, 'w', 'w'},                         // P2 B4
    {26, HIGH, 0, 'e', 'e'},                         // P2 B5
    {28, HIGH, 0, 'y', 'y'},                         // P2 B6
};

boolean startBlock = false;  // Block shift key
boolean enableState = false; // Enable state
boolean prevEnState = false; // Previous enable state
int periodSum = 0;           // Sum of periods
int sCounter = 0;            // Samples counter

void setup()
{
  for (int i = 0; i < SW_INPUTS; i++)
  {
    pinMode(inputs[i].pin, INPUT_PULLUP);         // Set input pin
    inputs[i].state = digitalRead(inputs[i].pin); // Read state
    inputs[i].dbTime = millis();                  // Init debounce time
  }
  pinMode(HSyncPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(disablePin, OUTPUT);
#ifdef SYNC_MONITOR_ACTIVE
  digitalWrite(disablePin, HIGH); // Disable sync monitor
  digitalWrite(ledPin, LOW);      // LED off
#else
  digitalWrite(disablePin, LOW); // Enable sync monitor
  digitalWrite(ledPin, HIGH);    // LED on
#endif
  Keyboard.begin(); // Init Keyboard library
}

void loop()
{
  for (int i = 0; i < SW_INPUTS; i++)
  {
    handleInput(&inputs[i], i == 0);
  }
#ifdef SYNC_MONITOR_ACTIVE
  freqBlock(); // Frequency block
#endif
}

void handleInput(digitalInput *input, boolean isShift)
{
  // If time to check state
  if (millis() - input->dbTime > debTime &&
      digitalRead(input->pin) != input->state)
  {
    input->dbTime = millis();               // Update debounce time
    input->state = digitalRead(input->pin); // Update state

    // Use btn if current is shift OR state is HIGH, else use btn_shift
    uint8_t btn =
        (isShift || inputs[0].state == HIGH) ? input->btn : input->btn_shift;

    // If valid key, press/release key depending on state
    if (btn > 32)
    {
      input->state == LOW ? Keyboard.press(btn) : Keyboard.release(btn);
      delay(8); // Delay for stable input
    }

    // Block/unblock shift depending on state
    startBlock = (isShift && input->state == LOW) ? true : startBlock;
    if (isShift && input->state == HIGH && startBlock)
    {
      startBlock = false;
    }
  }
}

void freqBlock()
{
  int periodIst = pulseIn(HSyncPin, HIGH);
  periodSum += periodIst;
  sCounter++;
  if (sCounter > samples)
  {
    int periodAWG = periodSum / sCounter; // Average period
    periodSum = 0;                        // Reset sum
    sCounter = 0;                         // Reset counter
    enableState = (periodAWG > DEFAULT_FREQ);
    // If state changed
    if (enableState != prevEnState)
    {
      prevEnState = enableState;
      digitalWrite(disablePin, !enableState); // Update disable pin
      digitalWrite(ledPin, enableState);      // Update LED pin
    }
  }
}
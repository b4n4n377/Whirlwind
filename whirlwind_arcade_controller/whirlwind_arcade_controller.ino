#include "Arduino.h"
#include "Adafruit_TinyUSB.h"
#include "hid_desc.hpp"

// Define necessary constants
#define LED_PIN PICO_DEFAULT_LED_PIN // Default LED Pin on the Pico
#define DEBOUNCE_MS 8ul // Milliseconds to debounce
#define PICO_GPIO_MASK 0x1C7FFFFF // GPIO mask for the Pico
#define PICO_GPIO_COUNT 26 // GPIO count for the Pico

#define LED_ENABLED // Comment to disable the LED
#define WAKE_ENABLED // Comment to disable waking from sleep

// Register address for resetting the Pico
#define AIRCR_Register (*((volatile uint32_t*)(PPB_BASE + 0x0ED0C)))
#define REBOOT AIRCR_Register = 0x5FA0004; // Reset command

// Initialize USB HID with product and vendor IDs
Adafruit_USBD_HID usb_hid(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_NONE, 1, false);

// Button to GPIO array
// Maps each button to a GPIO pin for both gamepads
uint16_t buttons[PICO_GPIO_COUNT] =
{
	// Gamepad A
	8,10,16,14,           // up, down, left, right
	12,20,18,22,24,28, // buttons 1-6
	2,4,6,           // buttons 7,8,9

	// Gamepad B
	9,11,17,15,           // up, down, left, right
	13,21,19,23,26,27, // buttons 1-6
	3,5,7            // buttons 7,8,9
};

// Create a struct to hold all information about a gamepad
struct GamepadInfo {
	GamepadReport report; // Gamepad report to send over USB
	bool needToSend = false; // Whether the gamepad state has changed
} gamepads[2]; // Create an array of gamepads

uint32_t timers[PICO_GPIO_COUNT] = {0}; // Initialize timers for each button
uint32_t states[PICO_GPIO_COUNT] = {0}; // Initialize button states

uint32_t gpio_now = 0; // Initialize current GPIO state
uint32_t time_now = 0; // Initialize current time

// Setup function called once when the program starts
void setup()
{
    Serial.end(); // Close the serial port
    TinyUSBDevice.setID(0x16c0, 0x05e1); // Set the USB vendor and product IDs
    TinyUSBDevice.setProductDescriptor("Pico Dual Gamepad"); // Set the product description
    gpio_init_mask(PICO_GPIO_MASK); // Initialize the GPIOs according to the mask
    gpio_set_dir_masked(PICO_GPIO_MASK, GPIO_IN); // Set the direction of the GPIOs to input

    // Enable pull-up resistors on all inputs
    for (int i = 0; i <= 28; i++)
        if (PICO_GPIO_MASK & (1ul << i)) gpio_pull_up(i);

    gpio_init(LED_PIN); // Initialize the LED pin
    gpio_set_dir(LED_PIN, GPIO_OUT); // Set the LED pin as output

    usb_hid.begin(); // Begin the USB HID communication
    while(!TinyUSBDevice.mounted()) sleep_ms(1); // Wait until the device is mounted
}

// Loop function called continuously when the program runs
void loop()
{
    gpio_now = ~gpio_get_all(); // Get the inverse of the GPIO states (because of pull-up resistors)
    gpio_now &= PICO_GPIO_MASK; // Apply the GPIO mask
    time_now = time_us_64() / 1000; // Get the current time in milliseconds

#ifdef LED_ENABLED
    gpio_put(LED_PIN, gpio_now != 0); // Set the LED state based on whether any button is pressed
#endif

#ifdef WAKE_ENABLED
    // If the USB is suspended and any button is pressed
    if (TinyUSBDevice.suspended() && gpio_now)
    {
        TinyUSBDevice.remoteWakeup(); // Send a remote wakeup command to the host
        REBOOT; // Reboot the Pico
        exit(0); // End the program
    }
#endif

    // For each button
    for (int i = 0; i < PICO_GPIO_COUNT; i++)
    {
        // If the time since the last state change is more than the debounce time
        if (time_now - timers[i] > DEBOUNCE_MS)
        {
            uint32_t state = (gpio_now & (1ul << buttons[i])) ? 1 : 0; // Get the current button state
            // If the button state has changed
            if (state != states[i])
            {
                states[i] = state; // Update the button state
                timers[i] = time_now; // Update the timer
                gamepads[i < 13 ? 0 : 1].needToSend = true; // Mark the gamepad as needing to send a report
            }
        }
    }

    // For each gamepad
    for (int i = 0; i < 2; i++)
    {
        // If the gamepad needs to send a report
        if (gamepads[i].needToSend)
        {
            gamepads[i].report.buttonStates = 0; // Reset the button states
            // For each button in the gamepad
            for (int j = i*13; j < (i+1)*13; j++)
                gamepads[i].report.buttonStates |= states[j] << (j - i*13); // Update the button state
            
            while(!usb_hid.ready()); // Wait until the USB HID is ready
            // Send the gamepad report over USB HID
            usb_hid.sendReport(REPORT_ID_GAMEPAD_A + i, &gamepads[i].report, sizeof(GamepadReport));
            gamepads[i].needToSend = false; // Reset the needToSend flag
        }
    }
}

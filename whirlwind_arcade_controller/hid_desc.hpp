// Include the Adafruit TinyUSB library for handling USB communications.
#include "Adafruit_TinyUSB.h"

// Include the standard int header to have access to standard integer types.
#include <stdint.h>

// Define a HID Gamepad Descriptor macro that describes the format of HID reports that the device sends and receives.
// HID_USAGE_PAGE and HID_USAGE define what kind of device is being used - in this case, a gamepad on the desktop.
// HID_COLLECTION defines the type of data that is being grouped - in this case, an application collection which is a group of main items that describe the device.
// HID_USAGE_PAGE_BUTTON describes that the following data corresponds to button inputs.
// HID_USAGE_MIN and HID_USAGE_MAX define the minimum and maximum usage ID values, which are the possible button indices in this case.
// HID_LOGICAL_MIN and HID_LOGICAL_MAX define the minimum and maximum logical values for the data, which are the possible button states in this case.
// HID_REPORT_COUNT defines the maximum number of items that can be reported - in this case, 32 buttons.
// HID_REPORT_SIZE defines the size of each item in bits - in this case, 1 bit per button.
// HID_INPUT defines the data type, whether the data is variable or an array, and whether the data is absolute or relative.
#define HID_GAMEPAD_DESCRIPTOR(...) \
	HID_USAGE_PAGE   (HID_USAGE_PAGE_DESKTOP                ),\
	HID_USAGE        (HID_USAGE_DESKTOP_GAMEPAD             ),\
	HID_COLLECTION   (HID_COLLECTION_APPLICATION            ),\
	__VA_ARGS__ \
	HID_USAGE_PAGE   (HID_USAGE_PAGE_BUTTON                 ),\
	HID_USAGE_MIN    (1                                     ),\
	HID_USAGE_MAX    (32                                    ),\
	HID_LOGICAL_MIN  (0                                     ),\
	HID_LOGICAL_MAX  (1                                     ),\
	HID_REPORT_COUNT (32                                    ),\
	HID_REPORT_SIZE  (1                                     ),\
	HID_INPUT        (HID_DATA | HID_VARIABLE | HID_ABSOLUTE),\
	HID_COLLECTION_END \

// Define Report IDs
// Report IDs are used to distinguish between different types of reports that a device can send or receive.
enum
{
	REPORT_ID_GAMEPAD_A = 1,
	REPORT_ID_GAMEPAD_B
};

// Define HID report descriptor using TinyUSB's template.
// This descriptor describes the format of the reports that the device will send and receive.
// Two gamepad reports are created with the REPORT_ID_GAMEPAD_A and REPORT_ID_GAMEPAD_B identifiers.
uint8_t const desc_hid_report[] =
{
	HID_GAMEPAD_DESCRIPTOR(HID_REPORT_ID(REPORT_ID_GAMEPAD_A)),
	HID_GAMEPAD_DESCRIPTOR(HID_REPORT_ID(REPORT_ID_GAMEPAD_B))
};

// Define a struct to hold the data that will be sent in a gamepad report.
// This struct contains a field for the button states, where each bit represents a different button.
// The TU_ATTR_PACKED attribute is used to ensure that the compiler does not add padding between the struct's fields, which could lead to unexpected results when sending and receiving reports.
typedef struct TU_ATTR_PACKED
{
	uint32_t buttonStates;  // Button mask for currently pressed buttons
} GamepadReport;

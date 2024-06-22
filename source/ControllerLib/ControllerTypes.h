#pragma once

#define CONTROLLER_MAX_INPUTS             4
#define CONTROLLER_INPUT_BUFFER_SIZE      128
#define CONTROLLER_HID_REPORT_BUFFER_SIZE 512

enum ControllerFeature : uint8_t
{
    SUPPORTS_RUMBLE,
    SUPPORTS_COUNT
};

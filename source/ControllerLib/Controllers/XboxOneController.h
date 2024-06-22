#pragma once

#include "BaseController.h"

// References used:
// https://github.com/quantus/xbox-one-controller-protocol
// https://cs.chromium.org/chromium/src/device/gamepad/xbox_controller_mac.mm

struct XboxOneButtonData
{
    uint8_t type;
    uint8_t const_0;
    uint16_t id;

    bool button5 : 1; // sync
    bool dummy : 1;   // dummy - Always 0
    bool button6 : 1;
    bool button7 : 1;

    bool button1 : 1;
    bool button2 : 1;
    bool button3 : 1;
    bool button4 : 1;

    bool dpad_up : 1;
    bool dpad_down : 1;
    bool dpad_left : 1;
    bool dpad_right : 1;

    bool button8 : 1;
    bool button9 : 1;
    bool button10 : 1;
    bool button11 : 1;

    uint16_t trigger_left;
    uint16_t trigger_right;

    int16_t stick_left_x;
    int16_t stick_left_y;
    int16_t stick_right_x;
    int16_t stick_right_y;
};

class XboxOneController : public BaseController
{
private:
    RawInputData m_rawInput;
    ams::Result SendInitBytes(uint16_t input_idx);
    ams::Result WriteAckModeReport(uint16_t input_idx, uint8_t sequence);

public:
    XboxOneController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger);
    virtual ~XboxOneController() override;

    virtual ams::Result Initialize() override;

    virtual ams::Result ReadInput(RawInputData *rawData, uint16_t *input_idx, uint32_t timeout_us) override;

    bool Support(ControllerFeature feature) override;

    ams::Result SetRumble(uint16_t input_idx, float amp_high, float amp_low) override;
};
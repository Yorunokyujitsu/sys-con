#pragma once

#include "BaseController.h"

class HIDJoystick;

class GenericHIDController : public BaseController
{
private:
    std::shared_ptr<HIDJoystick> m_joystick;
    uint8_t m_joystick_count = 0;

public:
    GenericHIDController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger);
    virtual ~GenericHIDController() override;

    virtual ams::Result Initialize() override;

    virtual uint16_t GetInputCount() override;

    virtual ams::Result ReadInput(RawInputData *rawData, uint16_t *input_idx, uint32_t timeout_us) override;
};
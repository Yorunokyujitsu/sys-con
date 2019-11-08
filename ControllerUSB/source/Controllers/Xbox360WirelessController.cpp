#include "Controllers/Xbox360WirelessController.h"
#include <cmath>

static ControllerConfig _xbox360WControllerConfig{};

Xbox360WirelessController::Xbox360WirelessController(std::unique_ptr<IUSBDevice> &&interface)
    : IController(std::move(interface))
{
}

Xbox360WirelessController::~Xbox360WirelessController()
{
    Exit();
}

Status Xbox360WirelessController::Initialize()
{
    Status rc;

    rc = OpenInterfaces();
    if (S_FAILED(rc))
        return rc;

    rc = ReconnectController();
    if (S_FAILED(rc))
        return rc;

    SetLED(XBOX360LED_TOPLEFT);
    return rc;
}
void Xbox360WirelessController::Exit()
{
    CloseInterfaces();
}

Status Xbox360WirelessController::OpenInterfaces()
{
    Status rc;
    rc = m_device->Open();
    if (S_FAILED(rc))
        return rc;

    //This will open each interface and try to acquire Xbox One controller's in and out endpoints, if it hasn't already
    std::vector<std::unique_ptr<IUSBInterface>> &interfaces = m_device->GetInterfaces();
    for (auto &&interface : interfaces)
    {
        rc = interface->Open();
        if (S_FAILED(rc))
            return rc;

        if (interface->GetDescriptor()->bInterfaceProtocol != 129)
            continue;

        if (interface->GetDescriptor()->bNumEndpoints < 2)
            continue;

        if (!m_inPipe)
        {
            for (int i = 0; i != 15; ++i)
            {
                IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, i);
                if (inEndpoint)
                {
                    rc = inEndpoint->Open();
                    if (S_FAILED(rc))
                        return 55555;

                    m_inPipe = inEndpoint;
                    break;
                }
            }
        }

        if (!m_outPipe)
        {
            for (int i = 0; i != 15; ++i)
            {
                IUSBEndpoint *outEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_OUT, i);
                if (outEndpoint)
                {
                    rc = outEndpoint->Open();
                    if (S_FAILED(rc))
                        return 66666;

                    m_outPipe = outEndpoint;
                    break;
                }
            }
        }
    }

    if (!m_inPipe || !m_outPipe)
        return 3469;

    return rc;
}
void Xbox360WirelessController::CloseInterfaces()
{
    if (m_presence)
        PowerOffController();

    //m_device->Reset();
    m_device->Close();
}

Status Xbox360WirelessController::GetInput()
{
    uint8_t input_bytes[64];

    Status rc = m_inPipe->Read(input_bytes, sizeof(input_bytes));

    uint8_t type = input_bytes[0];

    if (input_bytes[0] & 0x08)
    {
        bool newPresence = (input_bytes[1] & 0x80) != 0;

        if (m_presence != newPresence)
        {
            m_presence = newPresence;

            if (m_presence)
                ReconnectController();
            else
                PowerOffController();
        }
    }

    if (input_bytes[1] != 0x1)
        return 1;

    if (type == XBOX360INPUT_BUTTON)
    {
        m_buttonData = *reinterpret_cast<Xbox360ButtonData *>(input_bytes);
    }

    return rc;
}

float Xbox360WirelessController::NormalizeTrigger(uint8_t value)
{
    uint16_t deadzone = (UINT8_MAX * _xbox360WControllerConfig.triggerDeadzonePercent) / 100;
    //If the given value is below the trigger zone, save the calc and return 0, otherwise adjust the value to the deadzone
    return value < deadzone
               ? 0
               : static_cast<float>(value - deadzone) / (UINT8_MAX - deadzone);
}

void Xbox360WirelessController::NormalizeAxis(int16_t x,
                                              int16_t y,
                                              uint8_t deadzonePercent,
                                              float *x_out,
                                              float *y_out)
{
    float x_val = x;
    float y_val = y;
    // Determine how far the stick is pushed.
    //This will never exceed 32767 because if the stick is
    //horizontally maxed in one direction, vertically it must be neutral(0) and vice versa
    float real_magnitude = std::sqrt(x_val * x_val + y_val * y_val);
    float real_deadzone = (32767 * deadzonePercent) / 100;
    // Check if the controller is outside a circular dead zone.
    if (real_magnitude > real_deadzone)
    {
        // Clip the magnitude at its expected maximum value.
        float magnitude = std::min(32767.0f, real_magnitude);
        // Adjust magnitude relative to the end of the dead zone.
        magnitude -= real_deadzone;
        // Normalize the magnitude with respect to its expected range giving a
        // magnitude value of 0.0 to 1.0
        //ratio = (currentValue / maxValue) / realValue
        float ratio = (magnitude / (32767 - real_deadzone)) / real_magnitude;
        // Y is negated because xbox controllers have an opposite sign from
        // the 'standard controller' recommendations.
        *x_out = x_val * ratio;
        *y_out = y_val * ratio;
    }
    else
    {
        // If the controller is in the deadzone zero out the magnitude.
        *x_out = *y_out = 0.0f;
    }
}

//Pass by value should hopefully be optimized away by RVO
NormalizedButtonData Xbox360WirelessController::GetNormalizedButtonData()
{
    NormalizedButtonData normalData;

    normalData.triggers[0] = NormalizeTrigger(m_buttonData.trigger_left);
    normalData.triggers[1] = NormalizeTrigger(m_buttonData.trigger_right);

    NormalizeAxis(m_buttonData.stick_left_x, m_buttonData.stick_left_y, _xbox360WControllerConfig.leftStickDeadzonePercent,
                  &normalData.sticks[0].axis_x, &normalData.sticks[0].axis_y);
    NormalizeAxis(m_buttonData.stick_right_x, m_buttonData.stick_right_y, _xbox360WControllerConfig.rightStickDeadzonePercent,
                  &normalData.sticks[1].axis_x, &normalData.sticks[1].axis_y);

    bool buttons[NUM_CONTROLLERBUTTONS]{
        m_buttonData.y,
        m_buttonData.b,
        m_buttonData.a,
        m_buttonData.x,
        m_buttonData.stick_left_click,
        m_buttonData.stick_right_click,
        m_buttonData.bumper_left,
        m_buttonData.bumper_right,
        normalData.triggers[0] > 0,
        normalData.triggers[1] > 0,
        m_buttonData.back,
        m_buttonData.start,
        m_buttonData.dpad_up,
        m_buttonData.dpad_right,
        m_buttonData.dpad_down,
        m_buttonData.dpad_left,
        false,
        m_buttonData.guide,
    };

    for (int i = 0; i != NUM_CONTROLLERBUTTONS; ++i)
    {
        ControllerButton button = _xbox360WControllerConfig.buttons[i];
        normalData.buttons[(button != NOT_SET ? button : i)] = buttons[i];
    }

    return normalData;
}

Status Xbox360WirelessController::SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
{
    uint8_t rumbleData[]{0x00, 0x01, 0x0F, 0xC0, 0x00, strong_magnitude, weak_magnitude, 0x00, 0x00, 0x00, 0x00, 0x00};
    return m_outPipe->Write(rumbleData, sizeof(rumbleData));
}

Status Xbox360WirelessController::SetLED(Xbox360LEDValue value)
{
    uint8_t ledPacket[]{0x00, 0x00, 0x08, static_cast<uint8_t>(value + 40), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return m_outPipe->Write(ledPacket, sizeof(ledPacket));
}

Status Xbox360WirelessController::PowerOffController()
{
    uint8_t poweroffPacket[]{0x00, 0x00, 0x08, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return m_outPipe->Write(poweroffPacket, sizeof(poweroffPacket));
}

Status Xbox360WirelessController::ReconnectController()
{
    uint8_t reconnectPacket[]{0x08, 0x00, 0x0F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return m_outPipe->Write(reconnectPacket, sizeof(reconnectPacket));
}

void Xbox360WirelessController::LoadConfig(const ControllerConfig *config)
{
    _xbox360WControllerConfig = *config;
}
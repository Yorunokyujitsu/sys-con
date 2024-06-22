#include "Controllers/Xbox360WirelessController.h"

// https://www.partsnotincluded.com/understanding-the-xbox-360-wired-controllers-usb-data/

static constexpr uint8_t reconnectPacket[]{0x08, 0x00, 0x0F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static constexpr uint8_t poweroffPacket[]{0x00, 0x00, 0x08, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static constexpr uint8_t initDriverPacket[]{0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

Xbox360WirelessController::Xbox360WirelessController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : BaseController(std::move(device), config, std::move(logger))
{
    for (int i = 0; i < XBOX360_MAX_INPUTS; i++)
        m_is_connected[i] = false;
}

Xbox360WirelessController::~Xbox360WirelessController()
{
}

ams::Result Xbox360WirelessController::OpenInterfaces()
{
    R_TRY(BaseController::OpenInterfaces());

    if (m_inPipe.size() < XBOX360_MAX_INPUTS)
    {
        LogPrint(LogLevelError, "Xbox360WirelessController: Not enough input endpoints (%d / %d)", m_inPipe.size(), XBOX360_MAX_INPUTS);
        R_RETURN(CONTROL_ERR_INVALID_ENDPOINT);
    }

    R_SUCCEED();
}

void Xbox360WirelessController::CloseInterfaces()
{
    for (int i = 0; i < XBOX360_MAX_INPUTS; i++)
    {
        if (m_is_connected[i])
            OnControllerDisconnect(i);
    }

    BaseController::CloseInterfaces();
}

ams::Result Xbox360WirelessController::ReadInput(RawInputData *rawData, uint16_t *input_idx, uint32_t timeout_us)
{
    uint8_t input_bytes[CONTROLLER_INPUT_BUFFER_SIZE];
    size_t size = sizeof(input_bytes);

    uint16_t controller_idx = m_current_controller_idx;
    m_current_controller_idx = (m_current_controller_idx + 1) % XBOX360_MAX_INPUTS;

    R_TRY(m_inPipe[controller_idx]->Read(input_bytes, &size, timeout_us));

    Xbox360ButtonData *buttonData = reinterpret_cast<Xbox360ButtonData *>(input_bytes);

    *input_idx = controller_idx;

    // https://github.com/xboxdrv/xboxdrv/blob/stable/src/xbox360_controller.cpp
    // https://github.com/felis/USB_Host_Shield_2.0/blob/master/XBOXRECV.cpp

    if (input_bytes[0] & 0x08) // Connect/Disconnect
    {
        bool is_connected = (input_bytes[1] & 0x80) != 0;

        if (m_is_connected[controller_idx] != is_connected)
        {
            if (is_connected)
                OnControllerConnect(controller_idx);
            else
                OnControllerDisconnect(controller_idx);
        }
    }
    else if (input_bytes[0] == 0x00 && input_bytes[1] == 0x01 && input_bytes[2] == 0x00 && input_bytes[3] == 0xf0)
    {
        buttonData = reinterpret_cast<Xbox360ButtonData *>(input_bytes + 4);

        if (buttonData->type == XBOX360INPUT_BUTTON) // Button data
        {

            rawData->buttons[1] = buttonData->button1;
            rawData->buttons[2] = buttonData->button2;
            rawData->buttons[3] = buttonData->button3;
            rawData->buttons[4] = buttonData->button4;
            rawData->buttons[5] = buttonData->button5;
            rawData->buttons[6] = buttonData->button6;
            rawData->buttons[7] = buttonData->button7;
            rawData->buttons[8] = buttonData->button8;
            rawData->buttons[9] = buttonData->button9;
            rawData->buttons[10] = buttonData->button10;
            rawData->buttons[11] = buttonData->button11;

            rawData->Rx = Normalize(buttonData->Rx, 0, 255);
            rawData->Ry = Normalize(buttonData->Ry, 0, 255);

            rawData->X = Normalize(buttonData->X, -32768, 32767);
            rawData->Y = Normalize(-buttonData->Y, -32768, 32767);
            rawData->Z = Normalize(buttonData->Z, -32768, 32767);
            rawData->Rz = Normalize(-buttonData->Rz, -32768, 32767);

            rawData->dpad_up = buttonData->dpad_up;
            rawData->dpad_right = buttonData->dpad_right;
            rawData->dpad_down = buttonData->dpad_down;
            rawData->dpad_left = buttonData->dpad_left;

            R_SUCCEED();
        }
    }

    R_RETURN(CONTROL_ERR_NOTHING_TODO);
}

bool Xbox360WirelessController::Support(ControllerFeature feature)
{
    if (feature == SUPPORTS_RUMBLE)
        return true;

    return false;
}

uint16_t Xbox360WirelessController::GetInputCount()
{
    return XBOX360_MAX_INPUTS;
}

ams::Result Xbox360WirelessController::SetRumble(uint16_t input_idx, float amp_high, float amp_low)
{
    uint8_t rumbleData[]{0x00, (uint8_t)(input_idx + 1), 0x0F, 0xC0, 0x00, (uint8_t)(amp_high * 255), (uint8_t)(amp_low * 255), 0x00, 0x00, 0x00, 0x00, 0x00};
    R_RETURN(m_outPipe[input_idx]->Write(rumbleData, sizeof(rumbleData)));
}

bool Xbox360WirelessController::IsControllerConnected(uint16_t input_idx)
{
    return m_is_connected[input_idx];
}

ams::Result Xbox360WirelessController::SetLED(uint16_t input_idx, Xbox360LEDValue value)
{
    uint8_t ledPacket[]{0x00, (uint8_t)(input_idx + 1), 0x08, (uint8_t)(value | 0x40), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    R_RETURN(m_outPipe[input_idx]->Write(ledPacket, sizeof(ledPacket)));
}

ams::Result Xbox360WirelessController::OnControllerConnect(uint16_t input_idx)
{
    LogPrint(LogLevelInfo, "Xbox360WirelessController Wireless controller connected (Idx: %d) ...", input_idx);

    m_outPipe[input_idx]->Write(reconnectPacket, sizeof(reconnectPacket));
    m_outPipe[input_idx]->Write(initDriverPacket, sizeof(initDriverPacket));

    SetLED(input_idx, (Xbox360LEDValue)((int)XBOX360LED_TOPLEFT + input_idx));

    m_is_connected[input_idx] = true;

    R_SUCCEED();
}

ams::Result Xbox360WirelessController::OnControllerDisconnect(uint16_t input_idx)
{
    LogPrint(LogLevelInfo, "Xbox360WirelessController Wireless controller disconnected (Idx: %d) ...", input_idx);

    m_outPipe[input_idx]->Write(poweroffPacket, sizeof(poweroffPacket));

    m_is_connected[input_idx] = false;

    R_SUCCEED();
}

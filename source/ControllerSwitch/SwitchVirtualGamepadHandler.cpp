#include "SwitchVirtualGamepadHandler.h"
#include "SwitchLogger.h"

SwitchVirtualGamepadHandler::SwitchVirtualGamepadHandler(std::unique_ptr<IController> &&controller, s32 polling_frequency_ms)
    : m_controller(std::move(controller)),
      m_polling_frequency_ms(std::max(1, polling_frequency_ms))
{
}

SwitchVirtualGamepadHandler::~SwitchVirtualGamepadHandler()
{
}

ams::Result SwitchVirtualGamepadHandler::Initialize()
{
    R_TRY(m_controller->Initialize())

    m_read_input_timeout_us = (m_polling_frequency_ms * 1000) / m_controller->GetInputCount();

    R_SUCCEED();
}

void SwitchVirtualGamepadHandler::Exit()
{
    ExitThread();

    m_controller->Exit();
}

void SwitchVirtualGamepadHandler::onRun()
{
    ::syscon::logger::LogDebug("SwitchVirtualGamepadHandler InputThread running ...");

    do
    {
        ams::TimeSpan startTimer = ams::os::ConvertToTimeSpan(ams::os::GetSystemTick());

        UpdateInput(m_read_input_timeout_us);
        UpdateOutput();

        s64 execution_time_us = (ams::os::ConvertToTimeSpan(ams::os::GetSystemTick()) - startTimer).GetMicroSeconds();

        /*
        if ((execution_time_us - m_read_input_timeout_us) > 1000)
            ::syscon::logger::LogError("SwitchVirtualGamepadHandler UpdateInputOutput took: %d us !", execution_time_us);
        */

        if (execution_time_us < m_read_input_timeout_us)
            svcSleepThread((m_read_input_timeout_us - execution_time_us) * 1000);

    } while (m_ThreadIsRunning);

    ::syscon::logger::LogDebug("SwitchVirtualGamepadHandler InputThread stopped !");
}

void SwitchVirtualGamepadHandlerThreadFunc(void *handler)
{
    static_cast<SwitchVirtualGamepadHandler *>(handler)->onRun();
}

ams::Result SwitchVirtualGamepadHandler::InitThread()
{
    m_ThreadIsRunning = true;
    R_ABORT_UNLESS(threadCreate(&m_Thread, &SwitchVirtualGamepadHandlerThreadFunc, this, thread_stack, sizeof(thread_stack), 0x30, -2));
    R_ABORT_UNLESS(threadStart(&m_Thread));
    return 0;
}

void SwitchVirtualGamepadHandler::ExitThread()
{
    m_ThreadIsRunning = false;
    svcCancelSynchronization(m_Thread.handle);
    threadWaitForExit(&m_Thread);
    threadClose(&m_Thread);
}

void SwitchVirtualGamepadHandler::ConvertAxisToSwitchAxis(float x, float y, s32 *x_out, s32 *y_out)
{
    float floatRange = 2.0f;
    float newRange = (JOYSTICK_MAX - JOYSTICK_MIN);

    *x_out = (((x + 1.0f) * newRange) / floatRange) + JOYSTICK_MIN;
    *y_out = -((((y + 1.0f) * newRange) / floatRange) + JOYSTICK_MIN);
}
#include "switch.h"
#include "logger.h"
#include <stratosphere.hpp>

#include "usb_module.h"
#include "controller_handler.h"
#include "config_handler.h"
#include "psc_module.h"
#include "version.h"
#include "SwitchHDLHandler.h"

// libstratosphere variables
namespace ams
{
    namespace syscon
    {

        namespace
        {

            alignas(0x40) constinit u8 g_heap_memory[512_KB];
            constinit lmem::HeapHandle g_heap_handle;
            constinit bool g_heap_initialized;
            constinit os::SdkMutex g_heap_init_mutex;

            lmem::HeapHandle GetHeapHandle()
            {
                if (AMS_UNLIKELY(!g_heap_initialized))
                {
                    std::scoped_lock lk(g_heap_init_mutex);

                    if (AMS_LIKELY(!g_heap_initialized))
                    {
                        g_heap_handle = lmem::CreateExpHeap(g_heap_memory, sizeof(g_heap_memory), lmem::CreateOption_ThreadSafe);
                        g_heap_initialized = true;
                    }
                }

                return g_heap_handle;
            }

            void *Allocate(size_t size)
            {
                return lmem::AllocateFromExpHeap(GetHeapHandle(), size);
            }

            void *AllocateWithAlign(size_t size, size_t align)
            {
                return lmem::AllocateFromExpHeap(GetHeapHandle(), size, align);
            }

            void Deallocate(void *p, size_t size)
            {
                AMS_UNUSED(size);
                return lmem::FreeToExpHeap(GetHeapHandle(), p);
            }

        } // namespace

    } // namespace syscon

    namespace init
    {
        alignas(0x1000) constinit u8 g_hdls_buffer[64_KB];

        void InitializeSystemModuleBeforeConstructors(void)
        {
            R_ABORT_UNLESS(sm::Initialize());

            fs::InitializeForSystem();
            fs::SetAllocator(syscon::Allocate, syscon::Deallocate);
            fs::SetEnabledAutoAbort(false);

            R_ABORT_UNLESS(usbHsInitialize());
            R_ABORT_UNLESS(pscmInitialize());
            R_ABORT_UNLESS(setsysInitialize());

            // Initialize system firmware version
            SetSysFirmwareVersion fw;
            R_ABORT_UNLESS(setsysGetFirmwareVersion(&fw));
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));

            R_ABORT_UNLESS(hiddbgInitialize());
            if (hosversionAtLeast(7, 0, 0))
                R_ABORT_UNLESS(hiddbgAttachHdlsWorkBuffer(&SwitchHDLHandler::GetHdlsSessionId(), g_hdls_buffer, sizeof(g_hdls_buffer)));

            R_ABORT_UNLESS(hidInitialize());
            hidInitializeNpad();
            // R_ABORT_UNLESS(hidSetSupportedNpadIdType(NpadIdTypes, NumNpadIdTypes));
            // R_ABORT_UNLESS(hidSetSupportedNpadStyleSet(HidNpadStyleSet_NpadStandard | HidNpadStyleTag_NpadSystemExt));

            ams::time::Initialize();

            R_ABORT_UNLESS(fs::MountSdCard("sdmc"));
        }

        void FinalizeSystemModule()
        {
            hiddbgReleaseHdlsWorkBuffer(SwitchHDLHandler::GetHdlsSessionId());
            hiddbgExit();
            usbHsExit();
            pscmExit();
        }

        void Startup()
        {
            /* ... */
        }
    } // namespace init

    void Main()
    {
        ::syscon::logger::Initialize(CONFIG_PATH "log.log");

        u32 version = hosversionGet();

        ::syscon::logger::LogInfo("-----------------------------------------------------");
        ::syscon::logger::LogInfo("SYS-CON started %s+%d-%s (Build date: %s %s)", ::syscon::version::syscon_tag, ::syscon::version::syscon_commit_count, ::syscon::version::syscon_git_hash, __DATE__, __TIME__);
        ::syscon::logger::LogInfo("OS version: %d.%d.%d (Built with Atmosphere %s)", HOSVER_MAJOR(version), HOSVER_MINOR(version), HOSVER_MICRO(version), ::syscon::version::atmosphere_version);

        ::syscon::logger::LogDebug("Initializing configuration ...");

        ::syscon::config::GlobalConfig globalConfig;
        ::syscon::config::LoadGlobalConfig(&globalConfig);

        ::syscon::logger::SetLogLevel(globalConfig.log_level);

        ::syscon::logger::LogDebug("Initializing controllers ...");
        ::syscon::controllers::Initialize();

        // Reduce polling frequency when we use debug or trace to avoid spamming the logs
        if (globalConfig.log_level == LOG_LEVEL_TRACE && globalConfig.polling_frequency_ms < 500)
            globalConfig.polling_frequency_ms = 500;

        if (globalConfig.log_level == LOG_LEVEL_DEBUG && globalConfig.polling_frequency_ms < 100)
            globalConfig.polling_frequency_ms = 100;

        ::syscon::logger::LogDebug("Polling frequency: %d ms", globalConfig.polling_frequency_ms);
        ::syscon::controllers::SetPollingParameters(globalConfig.polling_frequency_ms, globalConfig.polling_thread_priority);

        ::syscon::logger::LogDebug("Initializing USB stack ...");
        ::syscon::usb::Initialize(globalConfig.discovery_mode, globalConfig.discovery_vidpid, globalConfig.auto_add_controller);

        ::syscon::logger::LogDebug("Initializing power supply managment ...");
        ::syscon::psc::Initialize();

        while (true)
        {
            svcSleepThread(1e+8L);
        }

        ::syscon::psc::Exit();
        ::syscon::usb::Exit();
        ::syscon::controllers::Exit();
        ::syscon::logger::Exit();
    }

} // namespace ams

void *operator new(size_t size)
{
    return ams::syscon::Allocate(size);
}

void *operator new(size_t size, const std::nothrow_t &)
{
    return ams::syscon::Allocate(size);
}

void operator delete(void *p)
{
    return ams::syscon::Deallocate(p, 0);
}

void operator delete(void *p, size_t size)
{
    return ams::syscon::Deallocate(p, size);
}

void *operator new[](size_t size)
{
    return ams::syscon::Allocate(size);
}

void *operator new[](size_t size, const std::nothrow_t &)
{
    return ams::syscon::Allocate(size);
}

void operator delete[](void *p)
{
    return ams::syscon::Deallocate(p, 0);
}

void operator delete[](void *p, size_t size)
{
    return ams::syscon::Deallocate(p, size);
}

void *operator new(size_t size, std::align_val_t align)
{
    return ams::syscon::AllocateWithAlign(size, static_cast<size_t>(align));
}

void operator delete(void *p, std::align_val_t align)
{
    AMS_UNUSED(align);
    return ams::syscon::Deallocate(p, 0);
}
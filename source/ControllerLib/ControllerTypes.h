#pragma once

enum ControllerType : uint8_t
{
    CONTROLLER_UNDEFINED,
    CONTROLLER_XBOX360,
    CONTROLLER_XBOX360W,
    CONTROLLER_XBOXONE,
    CONTROLLER_XBOXONEW,
    CONTROLLER_DUALSHOCK3,
    CONTROLLER_DUALSHOCK4,
};

enum VendorIDs : uint16_t
{
    VENDOR_MICROSOFT = 0x45e,
    VENDOR_SONY = 0x54c,
};

enum ProductIDs : uint16_t
{
    PRODUCT_XBOX360 = 0x28e,
    /*
    PRODUCT_XBOX360_WIRELESS = 0x28F,
    PRODUCT_XBOX360_WIRELESS_MODULE = 0x765,
    PRODUCT_XBOX360_WIRELESS_ADAPTER = 0x719,
    PRODUCT_XBOX360_WIRELESS_N_ADAPTER = 0x2A8,
    PRODUCT_XBOX360_WIRELESS_NETWORK_ADAPTER = 0x292,
    PRODUCT_XBOX360_WIRELESS_RECEIVER = 0x2A1,
    PRODUCT_XBOX360_WIRELESS_RECEIVER_2 = 0x291,
    */
    PRODUCT_XBOXONE2013 = 0x2d1,
    PRODUCT_XBOXONE2015 = 0x2dd,
    PRODUCT_XBOXONEELITE = 0x2e3,
    PRODUCT_XBOXONES = 0x2ea,
    PRODUCT_XBOXADAPTIVE = 0xb0a,

    PRODUCT_DUALSHOCK3 = 0x268,
    PRODUCT_DUALSHOCK4_1X = 0x5c4,
    PRODUCT_DUALSHOCK4_2X = 0x9cc,
};

enum ControllerSupport : uint8_t
{
    SUPPORTS_RUMBLE,
    SUPPORTS_BLUETOOTH,
    SUPPORTS_PAIRING,
    SUPPORTS_NOTHING,
    SUPPORTS_SIXAXIS,
    SUPPORTS_SEVENAXIS,
    SUPPORTS_PRESSUREBUTTONS,
};
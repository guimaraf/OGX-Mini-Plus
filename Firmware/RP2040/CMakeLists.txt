cmake_minimum_required(VERSION 3.13)

include(${CMAKE_CURRENT_LIST_DIR}/../FWDefines.cmake)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SRC ${CMAKE_CURRENT_LIST_DIR}/src)
set(EXTERNAL_DIR ${CMAKE_CURRENT_LIST_DIR}/../external)
set(PICOSDK_VERSION_TAG "2.1.0")

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/init_submodules.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/patch_libs.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/generate_gatt_header.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/get_pico_sdk.cmake)

set(PICO_PIO_USB_PATH ${EXTERNAL_DIR}/Pico-PIO-USB)
set(PICO_TINYUSB_PATH ${EXTERNAL_DIR}/tinyusb)
set(BLUEPAD32_ROOT ${EXTERNAL_DIR}/bluepad32)
set(BTSTACK_ROOT ${BLUEPAD32_ROOT}/external/btstack)
set(PICO_BTSTACK_PATH ${BTSTACK_ROOT})
set(LIBFIXMATH_PATH ${EXTERNAL_DIR}/libfixmath)

get_pico_sdk(${EXTERNAL_DIR} ${PICOSDK_VERSION_TAG})
init_git_submodules(${EXTERNAL_DIR}
    ${BLUEPAD32_ROOT}
    ${PICO_TINYUSB_PATH}
)
apply_lib_patches(${EXTERNAL_DIR})

set(SOURCES_BOARD 
    ${SRC}/main.cpp

    ${SRC}/OGXMini/OGXMini.cpp
    ${SRC}/OGXMini/Board/Standard.cpp
    ${SRC}/OGXMini/Board/PicoW.cpp
    ${SRC}/OGXMini/Board/Four_Channel_I2C.cpp
    ${SRC}/OGXMini/Board/ESP32_Bluepad32_I2C.cpp
    ${SRC}/OGXMini/Board/ESP32_Blueretro_I2C.cpp

    ${SRC}/TaskQueue/TaskQueue.cpp

    ${SRC}/Board/ogxm_log.cpp
    ${SRC}/Board/esp32_api.cpp
    ${SRC}/Board/board_api.cpp
    ${SRC}/Board/board_api_private/board_api_led.cpp
    ${SRC}/Board/board_api_private/board_api_rgb.cpp
    ${SRC}/Board/board_api_private/board_api_bt.cpp
    ${SRC}/Board/board_api_private/board_api_usbh.cpp
    
    ${SRC}/UserSettings/UserSettings.cpp
    ${SRC}/UserSettings/UserProfile.cpp
    ${SRC}/UserSettings/JoystickSettings.cpp
    ${SRC}/UserSettings/TriggerSettings.cpp

    ${SRC}/USBDevice/tud_callbacks.cpp
    ${SRC}/USBDevice/DeviceManager.cpp
    ${SRC}/USBDevice/DeviceDriver/DeviceDriver.cpp
    ${SRC}/USBDevice/DeviceDriver/PSClassic/PSClassic.cpp
    ${SRC}/USBDevice/DeviceDriver/PS3/PS3.cpp
    ${SRC}/USBDevice/DeviceDriver/DS4/DS4.cpp
    ${SRC}/USBDevice/DeviceDriver/Switch/Switch.cpp
    ${SRC}/USBDevice/DeviceDriver/XInput/XInput.cpp
    ${SRC}/USBDevice/DeviceDriver/XInput/XInputGuitar360.cpp
    ${SRC}/USBDevice/DeviceDriver/XboxOG/XboxOG_GP.cpp
    ${SRC}/USBDevice/DeviceDriver/XboxOG/XboxOG_SB.cpp
    ${SRC}/USBDevice/DeviceDriver/XboxOG/XboxOG_XR.cpp
    ${SRC}/USBDevice/DeviceDriver/DInput/DInput.cpp
    ${SRC}/USBDevice/DeviceDriver/WebApp/WebApp.cpp
    ${SRC}/USBDevice/DeviceDriver/XInput/tud_xinput/tud_xinput.cpp
    ${SRC}/USBDevice/DeviceDriver/XboxOG/tud_xid/tud_xid.cpp
)
set(LIBS_BOARD
    # Base
    pico_stdlib
    pico_multicore
    pico_rand
    hardware_timer
    hardware_clocks
    hardware_flash
    tinyusb_device
    tinyusb_board
    # UART
    hardware_uart
    hardware_irq
    #fix16
    libfixmath
)
set(INC_DIRS_BOARD
)

# Config options

set(MAX_GAMEPADS 1 CACHE STRING "Set number of gamepads, 1 to 4")
if (MAX_GAMEPADS GREATER 4 OR MAX_GAMEPADS LESS 1)
    message(FATAL_ERROR "MAX_GAMEPADS must be between 1 and 4")
endif()
add_definitions(-DMAX_GAMEPADS=${MAX_GAMEPADS})

set(OGXM_BOARD "PI_PICO" CACHE STRING "Set board type, options can be found in src/board_config.h")
set(FLASH_SIZE_MB 2)
set(PICO_BOARD none)

if (OGXM_BOARD STREQUAL "PI_PICO")
    add_compile_definitions(CONFIG_OGXM_BOARD_PI_PICO=1)
    set(EN_USB_HOST TRUE)

elseif (OGXM_BOARD STREQUAL "PI_PICO2")
    add_compile_definitions(CONFIG_OGXM_BOARD_PI_PICO2=1)
    set(EN_USB_HOST TRUE)
    set(PICO_PLATFORM rp2350)
    set(FLASH_SIZE_MB 4)

elseif(OGXM_BOARD STREQUAL "PI_PICOW")
    add_compile_definitions(CONFIG_OGXM_BOARD_PI_PICOW=1)
    set(EN_BLUETOOTH TRUE)
    set(PICO_BOARD pico_w)

elseif(OGXM_BOARD STREQUAL "PI_PICO2W")
    add_compile_definitions(CONFIG_OGXM_BOARD_PI_PICO2W=1)
    set(EN_BLUETOOTH TRUE)
    set(PICO_BOARD pico2_w)
    set(PICO_PLATFORM rp2350)
    set(FLASH_SIZE_MB 4)

elseif(OGXM_BOARD STREQUAL "ADAFRUIT_FEATHER")
    add_compile_definitions(CONFIG_OGXM_BOARD_ADAFRUIT_FEATHER=1)
    set(EN_USB_HOST TRUE)
    set(EN_RGB TRUE)
    set(FLASH_SIZE_MB 8)

elseif(OGXM_BOARD STREQUAL "RP2040_ZERO")
    add_compile_definitions(CONFIG_OGXM_BOARD_RP2040_ZERO=1)
    set(EN_USB_HOST TRUE)
    set(EN_RGB TRUE)

# elseif(OGXM_BOARD STREQUAL "INTERNAL_4CH")
#     set(EN_USB_HOST TRUE)
#     set(EN_4CH TRUE)

elseif(OGXM_BOARD STREQUAL "EXTERNAL_4CH_I2C")
    add_compile_definitions(CONFIG_OGXM_BOARD_EXTERNAL_4CH=1)
    set(EN_USB_HOST TRUE)
    set(EN_4CH TRUE)
    set(EN_RGB TRUE)

elseif(OGXM_BOARD STREQUAL "ESP32_BLUEPAD32_I2C")
    add_compile_definitions(CONFIG_OGXM_BOARD_ESP32_BLUEPAD32_I2C=1)
    set(EN_ESP32 TRUE)
    set(EN_UART_BRIDGE TRUE)

    if(OGXM_RETAIL STREQUAL "TRUE")
        message(STATUS "Retail mode enabled.")
        add_compile_definitions(OGXM_RETAIL=1)
    endif()

elseif(OGXM_BOARD STREQUAL "ESP32_BLUERETRO_I2C")
    add_compile_definitions(CONFIG_OGXM_BOARD_ESP32_BLUERETRO_I2C=1)
    set(EN_ESP32 TRUE)
    set(EN_BLUERETRO_I2C TRUE)
    set(EN_UART_BRIDGE TRUE)

    if(OGXM_RETAIL STREQUAL "TRUE")
        message(STATUS "Retail mode enabled.")
        add_compile_definitions(OGXM_RETAIL=1)
    endif()

else()
    message(FATAL_ERROR "Invalid OGXM_BOARD value. See options in src/board_config.h")

endif()

add_definitions(-DOGXM_BOARD=${OGXM_BOARD})

if(EN_USB_HOST)
    message(STATUS "USB host enabled.")
    add_compile_definitions(CONFIG_EN_USB_HOST=1)
    list(APPEND SOURCES_BOARD
        ${SRC}/USBHost/tuh_callbacks.cpp
        # HID
        ${SRC}/USBHost/HostDriver/DInput/DInput.cpp
        ${SRC}/USBHost/HostDriver/PSClassic/PSClassic.cpp
        ${SRC}/USBHost/HostDriver/SwitchWired/SwitchWired.cpp
        ${SRC}/USBHost/HostDriver/SwitchPro/SwitchPro.cpp
        ${SRC}/USBHost/HostDriver/SwitchPro/SwitchProCloneRecovery.cpp
        ${SRC}/USBHost/HostDriver/SwitchPro/SwitchProRumble.cpp
        ${SRC}/USBHost/HostDriver/PS5/PS5.cpp
        ${SRC}/USBHost/HostDriver/PS4/PS4.cpp
        ${SRC}/USBHost/HostDriver/PS3/PS3.cpp
        ${SRC}/USBHost/HostDriver/N64/N64.cpp
        ${SRC}/USBHost/HostDriver/HIDGeneric/HIDGeneric.cpp

        ${SRC}/USBHost/HIDParser/HIDJoystick.cpp
        ${SRC}/USBHost/HIDParser/HIDReportDescriptor.cpp
        ${SRC}/USBHost/HIDParser/HIDReportDescriptorElements.cpp
        ${SRC}/USBHost/HIDParser/HIDReportDescriptorUsages.cpp
        ${SRC}/USBHost/HIDParser/HIDUtils.cpp

        # XInput
        ${SRC}/USBHost/HostDriver/XInput/XboxOG.cpp
        ${SRC}/USBHost/HostDriver/XInput/XboxOne.cpp
        ${SRC}/USBHost/HostDriver/XInput/Xbox360.cpp
        ${SRC}/USBHost/HostDriver/XInput/Xbox360W.cpp
        ${SRC}/USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.cpp
    )
    list(APPEND LIBS_BOARD
        tinyusb_host
        tinyusb_pico_pio_usb
    )
endif()

if(EN_BLUETOOTH)
    add_compile_definitions(CONFIG_EN_BLUETOOTH=1)
    message(STATUS "Bluetooth enabled.")

    generate_gatt_header(${BTSTACK_ROOT} ${SRC}/BLEServer/att_delayed_response.gatt ${SRC}/BLEServer/att_delayed_response.h)
    
    list(APPEND SOURCES_BOARD
        ${SRC}/Bluepad32/Bluepad32.cpp
        ${SRC}/BLEServer/BLEServer.cpp
    )
    list(APPEND INC_DIRS_BOARD
        ${SRC}
        ${BTSTACK_ROOT}/3rd-party/bluedroid/encoder/include
        ${BTSTACK_ROOT}/3rd-party/bluedroid/decoder/include
    )
    list(APPEND LIBS_BOARD
        pico_cyw43_arch_none
        pico_btstack_classic
        pico_btstack_cyw43
        bluepad32
    )
endif()

if(EN_RGB)
    add_compile_definitions(CONFIG_EN_RGB=1)
    message(STATUS "RGB enabled.")
    list(APPEND SOURCES_BOARD
        ${SRC}/Board/Pico_WS2812/WS2812.cpp
    )
    list(APPEND LIBS_BOARD
        hardware_pio
    )
endif()

if(EN_4CH)
    add_compile_definitions(CONFIG_EN_4CH=1)
    message(STATUS "4CH enabled.")
    # list(APPEND SOURCES_BOARD
    #     ${SRC}/I2CDriver/4Channel/I2CMaster.cpp
    #     ${SRC}/I2CDriver/4Channel/I2CSlave.cpp
    # )
    list(APPEND LIBS_BOARD
        hardware_i2c
        pico_i2c_slave
    )
endif()

if(EN_ESP32)
    add_compile_definitions(CONFIG_EN_ESP32=1)
    message(STATUS "ESP32 enabled.")
    if (EN_BLUERETRO_I2C)
        # Nothing
    else()
        list(APPEND LIBS_BOARD
            pico_i2c_slave
        )
    endif()
    list(APPEND LIBS_BOARD
        hardware_i2c
    )
endif()

if(EN_UART_BRIDGE)
    add_compile_definitions(CONFIG_EN_UART_BRIDGE=1)
    message(STATUS "UART bridge enabled.")
    list(APPEND SOURCES_BOARD
        ${SRC}/USBDevice/DeviceDriver/UARTBridge/UARTBridge.cpp
        ${SRC}/USBDevice/DeviceDriver/UARTBridge/uart_bridge/uart_bridge.c
    )
endif()

string(TIMESTAMP CURRENT_DATETIME "%Y-%m-%d %H:%M:%S")
add_compile_definitions(BUILD_DATETIME="${CURRENT_DATETIME}")
add_compile_definitions(FIRMWARE_NAME="${FW_NAME}")
add_compile_definitions(FIRMWARE_VERSION="${FW_VERSION}")
add_compile_definitions(PICO_FLASH_SIZE_BYTES=${FLASH_SIZE_MB}*1024*1024)
add_compile_definitions(NVS_SECTORS=4)

# Check for DVD dongle firmware
if(EXISTS ${SRC}/USBDevice/DeviceDriver/XboxOG/tud_xid/tud_xid_xremote_rom.h)
    message(STATUS "XRemote ROM available.")
    add_definitions(-DXREMOTE_ROM_AVAILABLE)
endif()

if(NOT EN_BLUETOOTH)
    add_compile_definitions(PICO_BOOT_STAGE2_CHOOSE_GENERIC_03H=1)
    add_compile_definitions(PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)
endif()

include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(${FW_NAME} C CXX ASM)

pico_sdk_init()

add_executable(${FW_NAME} ${SOURCES_BOARD})

set(BUILD_STR "")
option(OGXM_ENABLE_DEBUG_LOG "Enable OGXM debug logging without full Debug build" OFF)
option(OGXM_ENABLE_BT_RATE_DEBUG "Enable Bluetooth/USB rate telemetry logs" OFF)
option(OGXM_BLUEPAD32_SONY_FAST_INPUT "Skip Sony motion/touch parsing in Bluepad32 input hot path" ON)
option(OGXM_BLUEPAD32_DS4_DIRECT_INPUT "Bypass Bluepad32 remap/misc processing for DS4 Bluetooth input reports" ON)
option(OGXM_BLUEPAD32_DS4_L2CAP_RATE_DEBUG "Enable DS4 L2CAP interval telemetry inside Bluepad32" OFF)
option(OGXM_BLUEPAD32_DS4_CONTROL_ENABLE "Use JoypadOS-style DS4 SET_REPORT output on the control channel" ON)
option(OGXM_BLUEPAD32_DS4_DELAY_INITIAL_ENABLE "Delay the initial DS4 SET_REPORT that enables enhanced BT reports" OFF)
option(OGXM_BLUEPAD32_DS4_SKIP_INIT_CALIBRATION "Skip DS4 calibration GET_REPORT during initial BT bring-up" OFF)
option(OGXM_BLUEPAD32_DS4_SKIP_SDP "Skip SDP for DS4 Wireless Controller and connect with direct L2CAP" OFF)
option(OGXM_BLUEPAD32_SONY_L2CAP_AFTER_ENCRYPTION "For Sony BT controllers, create HID L2CAP channels only after ACL encryption is active" OFF)
option(OGXM_BLUEPAD32_DISABLE_SNIFF_FOR_BT_PERF_TEST "Disable Bluetooth Classic sniff mode while testing gamepad performance" ON)
option(OGXM_BLUEPAD32_FORCE_ACTIVE_LINK_POLICY "Apply active Bluetooth Classic link policy after each connection completes" ON)
option(OGXM_DISABLE_BLE_SERVER_FOR_BT_PERF_TEST "Disable local BLE server while testing Bluetooth gamepad performance" OFF)
option(OGXM_USB_TIGHT_POLL "Poll TinyUSB/device driver without the 1 ms sleep in the Pico W main loop" OFF)
option(OGXM_SWITCH_SEND_ON_CHANGE_ONLY "For Pokken/Switch USB mode, send HID input reports only when input changed or a pending report exists" ON)

function(configure_ogxm_debug_log target_name)
    set(TX_PIN 0)
    set(RX_PIN 1)
    set(UART_PORT)

    include(${CMAKE_CURRENT_LIST_DIR}/cmake/get_pico_uart_port.cmake)
    get_pico_uart_port(${TX_PIN} UART_PORT)

    message(STATUS "OGXM debug log enabled. UART port: ${UART_PORT}, TX: ${TX_PIN}, RX: ${RX_PIN}")

    target_compile_definitions(${target_name} PRIVATE
        OGXM_DEBUG=1
        PICO_DEFAULT_UART=${UART_PORT}
        PICO_DEFAULT_UART_TX_PIN=${TX_PIN}
        PICO_DEFAULT_UART_RX_PIN=${RX_PIN}
    )
endfunction()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(OGXM_BOARD STREQUAL "PI_PICOW" OR OGXM_BOARD STREQUAL "PI_PICO2W")
        message(FATAL_ERROR "Full Debug build will not work with the Pico W / Pico 2 W currently. Use OGXM_ENABLE_DEBUG_LOG=ON with a Release build instead.")
    endif()

    message(STATUS "Debug build enabled.")
    set(BUILD_STR "-Debug")

    pico_enable_stdio_uart(${FW_NAME} 1)
    configure_ogxm_debug_log(${FW_NAME})
 
    add_compile_definitions(CFG_TUSB_DEBUG=1)

    target_compile_options(${FW_NAME} PRIVATE
        -Wall                # Enable most warnings
        -Wextra              # Enable extra warnings
        -Wconversion         # Warn on type conversion issues
        -Wsign-conversion    # Warn on sign conversion issues
        -Wno-unused-parameter # Disable warnings for unused parameters
        # -Werror              # Treat warnings as errors
    )

elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    pico_enable_stdio_uart(${FW_NAME} 0)
    add_compile_definitions(CFG_TUSB_DEBUG=0)
    if(OGXM_ENABLE_DEBUG_LOG)
        set(BUILD_STR "-DebugLog")
        configure_ogxm_debug_log(${FW_NAME})
    endif()

    add_compile_options(
        -O3                     # Optimize for speed
        # -mcpu=cortex-m0plus     # Target ARM Cortex-M0+
        -mthumb                 # Use Thumb instruction set
        -ffunction-sections     # Place each function in its own section
        -fdata-sections         # Place each data item in its own section
        -fomit-frame-pointer    # Omit frame pointers
        -finline-functions      # Enable inlining
    )

    add_link_options(
        -Wl,--gc-sections       # Remove unused sections
        -flto                   # Link-Time Optimization
    )
endif()

if(OGXM_ENABLE_BT_RATE_DEBUG)
    if(NOT OGXM_ENABLE_DEBUG_LOG AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(WARNING "OGXM_ENABLE_BT_RATE_DEBUG=ON requires OGXM_ENABLE_DEBUG_LOG=ON or a Debug build to emit logs.")
    endif()
    target_compile_definitions(${FW_NAME} PRIVATE OGXM_BT_RATE_DEBUG=1)
endif()

if(OGXM_DISABLE_BLE_SERVER_FOR_BT_PERF_TEST)
    target_compile_definitions(${FW_NAME} PRIVATE OGXM_DISABLE_BLE_SERVER_FOR_BT_PERF_TEST=1)
endif()

if(OGXM_USB_TIGHT_POLL)
    target_compile_definitions(${FW_NAME} PRIVATE OGXM_USB_TIGHT_POLL=1)
endif()

if(OGXM_SWITCH_SEND_ON_CHANGE_ONLY)
    target_compile_definitions(${FW_NAME} PRIVATE OGXM_SWITCH_SEND_ON_CHANGE_ONLY=1)
endif()

pico_set_program_name(${FW_NAME} ${FW_NAME})
pico_set_program_version(${FW_NAME} ${FW_VERSION})

target_include_directories(${FW_NAME} PRIVATE 
    ${SRC}
)

if(EN_RGB)
    pico_generate_pio_header(${FW_NAME} ${SRC}/Board/Pico_WS2812/WS2812.pio)
endif()

include_directories(${INC_DIRS_BOARD})

if(EN_USB_HOST)
    add_subdirectory(${PICO_PIO_USB_PATH} ${CMAKE_BINARY_DIR}/Pico-PIO-USB)
endif()

if(EN_BLUETOOTH)
    add_subdirectory(${BLUEPAD32_ROOT}/src/components/bluepad32 libbluepad32)
    if(OGXM_BLUEPAD32_SONY_FAST_INPUT)
        target_compile_definitions(bluepad32 PRIVATE OGXM_BLUEPAD32_SONY_FAST_INPUT=1)
    endif()
    if(OGXM_BLUEPAD32_DS4_DIRECT_INPUT)
        target_compile_definitions(bluepad32 PRIVATE OGXM_BLUEPAD32_DS4_DIRECT_INPUT=1)
    endif()
    if(OGXM_BLUEPAD32_DS4_L2CAP_RATE_DEBUG)
        target_compile_definitions(bluepad32 PRIVATE OGXM_BLUEPAD32_DS4_L2CAP_RATE_DEBUG=1)
    endif()
    if(OGXM_BLUEPAD32_DS4_CONTROL_ENABLE)
        target_compile_definitions(bluepad32 PRIVATE OGXM_BLUEPAD32_DS4_CONTROL_ENABLE=1)
    endif()
    if(OGXM_BLUEPAD32_DS4_DELAY_INITIAL_ENABLE)
        target_compile_definitions(bluepad32 PRIVATE OGXM_BLUEPAD32_DS4_DELAY_INITIAL_ENABLE=1)
    endif()
    if(OGXM_BLUEPAD32_DS4_SKIP_INIT_CALIBRATION)
        target_compile_definitions(bluepad32 PRIVATE OGXM_BLUEPAD32_DS4_SKIP_INIT_CALIBRATION=1)
    endif()
    if(OGXM_BLUEPAD32_DS4_SKIP_SDP)
        target_compile_definitions(bluepad32 PRIVATE OGXM_BLUEPAD32_DS4_SKIP_SDP=1)
    endif()
    if(OGXM_BLUEPAD32_SONY_L2CAP_AFTER_ENCRYPTION)
        target_compile_definitions(bluepad32 PRIVATE OGXM_BLUEPAD32_SONY_L2CAP_AFTER_ENCRYPTION=1)
    endif()
    if(OGXM_BLUEPAD32_DISABLE_SNIFF_FOR_BT_PERF_TEST)
        target_compile_definitions(bluepad32 PRIVATE OGXM_BLUEPAD32_DISABLE_SNIFF_FOR_BT_PERF_TEST=1)
    endif()
    if(OGXM_BLUEPAD32_FORCE_ACTIVE_LINK_POLICY)
        target_compile_definitions(bluepad32 PRIVATE OGXM_BLUEPAD32_FORCE_ACTIVE_LINK_POLICY=1)
    endif()
endif()

add_subdirectory(${LIBFIXMATH_PATH} libfixmath)

target_link_libraries(${FW_NAME} PRIVATE ${LIBS_BOARD})

target_compile_definitions(libfixmath PRIVATE
    FIXMATH_FAST_SIN
    FIXMATH_NO_64BIT
    FIXMATH_NO_CACHE
    FIXMATH_NO_HARD_DIVISION
    FIXMATH_NO_OVERFLOW
    # FIXMATH_NO_ROUNDING
    # FIXMATH_OPTIMIZE_8BIT
)

if(OGXM_RETAIL STREQUAL "TRUE")
    set(EXE_FILENAME "${FW_NAME}-${FW_VERSION}-${OGXM_BOARD}${BUILD_STR}-Retail")
else()
    set(EXE_FILENAME "${FW_NAME}-${FW_VERSION}-${OGXM_BOARD}${BUILD_STR}")
endif()

set_target_properties(${FW_NAME} PROPERTIES OUTPUT_NAME ${EXE_FILENAME})

pico_add_extra_outputs(${FW_NAME})

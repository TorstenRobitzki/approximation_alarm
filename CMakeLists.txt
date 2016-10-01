cmake_minimum_required(VERSION 3.2)
project(approximation_alarm CXX ASM C)

set(CXX_DEFAULT_FLAGS "-Wall")
set(CXX_SPECIFIC_FLAGS "-std=c++11")

set(CXX_DISABLED_WARNINGS "-Wno-parentheses")

if (NOT DEFINED NRF51_SDK_ROOT)
    message( FATAL_ERROR " define NRF51_SDK_ROOT to point to the root of the SDK installation location!" )
endif()

# Stop compilation after first error
set(CXX_DEFAULT_FLAGS "${CXX_DEFAULT_FLAGS} -Wfatal-errors -Werror")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_CXX_COMPILER "arm-none-eabi-g++")
set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
set(CMAKE_ASM_COMPILER "arm-none-eabi-gcc")
set(CMAKE_C_COMPILER_FORCED 1)
set(CMAKE_CXX_COMPILER_FORCED 1)
set(ARM_FLAGS "--sysroot=/usr/local/arm/arm-none-eabi -mthumb -mcpu=cortex-m0")
set(ARM_FLAGS "${ARM_FLAGS} -ffunction-sections -fdata-sections")
set(CXX_ARC_FLAGS "${ARM_FLAGS} -mfloat-abi=soft")
set(LINK_ARC_FLAGS "${ARM_FLAGS} --specs=nosys.specs --specs=nano.specs -static -lm -lstdc++ -lsupc++")
set(CXX_SPECIFIC_FLAGS "${CXX_SPECIFIC_FLAGS} -ftemplate-backtrace-limit=0 -fvisibility-inlines-hidden -fno-rtti -fno-exceptions")
set(CXX_DISABLED_WARNINGS "${CXX_DISABLED_WARNINGS} -Wno-unused-local-typedefs")
set(LINK_DEFAULT_FLAGS "${LINK_DEFAULT_FLAGS} -Wl,--gc-sections -L ${CMAKE_SOURCE_DIR}/examples -nostdlib")

# debugging
set(CXX_BUILD_TYPE_FLAGS "-g -O3")
set(LINK_BUILD_TYPE_FLAGS "-g")

# device
set(NRF_SOFTDEVICE s130)
set(DEFAULT_JLINK_DEVICE_NAME nRF51422_xxAA)

string(TOUPPER ${NRF_SOFTDEVICE} NRF_SOFTDEVICE_UPPER)
set(NRF_SOFTDEVICE_HEX_FILE ${NRF51_SDK_ROOT}/components/softdevice/${NRF_SOFTDEVICE}/hex/${NRF_SOFTDEVICE}_nrf51_2.0.0_softdevice.hex)

message(WARNING ${NRF_SOFTDEVICE_HEX_FILE})
set(CMAKE_CXX_FLAGS "${CXX_DEFAULT_FLAGS} ${CXX_SPECIFIC_FLAGS} ${CXX_ARC_FLAGS} ${CXX_BUILD_TYPE_FLAGS} ${CXX_DISABLED_WARNINGS}")
set(CMAKE_C_FLAGS   "${CXX_DEFAULT_FLAGS} ${CXX_ARC_FLAGS} ${CXX_BUILD_TYPE_FLAGS} ${CXX_DISABLED_WARNINGS}")
set(CMAKE_EXE_LINKER_FLAGS "${LINK_DEFAULT_FLAGS} ${LINK_ARC_FLAGS} ${LINK_BUILD_TYPE_FLAGS} -L ${CMAKE_SOURCE_DIR}")

add_definitions(-D_LITTLE_ENDIAN=1)
add_definitions(-DSWI_DISABLE0)
add_definitions(-DSOFTDEVICE_PRESENT)
add_definitions(-DNRF51)
add_definitions(-DBLE_STACK_SUPPORT_REQD)
add_definitions(-DCONFIG_GPIO_AS_PINRESET)
add_definitions(-D__HEAP_SIZE=0)
add_definitions(-D${NRF_SOFTDEVICE_UPPER})
add_definitions(-DBSP_UART_SUPPORT)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${NRF51_SDK_ROOT}/components/softdevice/${NRF_SOFTDEVICE}/headers)
include_directories(${NRF51_SDK_ROOT}/components/toolchain/CMSIS/include)
include_directories(${NRF51_SDK_ROOT}/components/device)
include_directories(${NRF51_SDK_ROOT}/components/drivers_nrf/config)
include_directories(${NRF51_SDK_ROOT}/components/libraries/util)
include_directories(${NRF51_SDK_ROOT}/components/libraries/trace)
include_directories(${NRF51_SDK_ROOT}/components/ble/ble_db_discovery)
include_directories(${NRF51_SDK_ROOT}/components/ble/common)

if (DEFINED JLINK_DEVICE)
    message( WARNING "JLink ${JLINK_DEVICE} selected")
    set(SELECT_JLINK_DEVICE "usb ${JLINK_DEVICE}")
endif()

if (DEFINED NRFJPROG_DEVICE)
    message( WARNING "nrfjprog ${NRFJPROG_DEVICE} selected")
    set(SELECT_JLINK_DEVICE --snr ${NRFJPROG_DEVICE})
endif()

function(new_executable TARGETNAME)
    add_executable(${TARGETNAME}.elf
        ${TARGETNAME}.cpp
        runtime.cpp
        system_nrf51.c
        gcc_startup_nrf51.s
        ${NRF51_SDK_ROOT}/components/ble/ble_db_discovery/ble_db_discovery.c
        ${NRF51_SDK_ROOT}/components/libraries/util/app_error.c
        ${NRF51_SDK_ROOT}/components/softdevice/common/softdevice_handler/softdevice_handler.c
    )

    set_target_properties(${TARGETNAME}.elf PROPERTIES
        LINK_FLAGS "-T ${CMAKE_CURRENT_LIST_DIR}/armgcc_${NRF_SOFTDEVICE}_nrf51422_xxaa.ld -Wl,-Map -Wl,${TARGETNAME}.map")

    add_custom_target(${TARGETNAME}.bin
        arm-none-eabi-objcopy -S -O binary ${TARGETNAME}.elf ${TARGETNAME}.bin
        )
    add_custom_target(${TARGETNAME}.hex
        arm-none-eabi-objcopy -S -O ihex ${TARGETNAME}.elf ${TARGETNAME}.hex
        )
    add_dependencies(${TARGETNAME}.bin ${TARGETNAME}.elf)
    add_dependencies(${TARGETNAME}.hex ${TARGETNAME}.elf)

    add_custom_target(
       ${TARGETNAME}.jlink
       COMMAND
            echo "${SELECT_JLINK_DEVICE}" > ${TARGETNAME}.jlink &&
            echo "halt" >> ${TARGETNAME}.jlink &&
            echo "exec device = ${DEFAULT_JLINK_DEVICE_NAME}" >> ${TARGETNAME}.jlink &&
            echo "loadbin ${TARGETNAME}.bin, 0x1f000" >> ${TARGETNAME}.jlink &&
            echo "r" >> ${TARGETNAME}.jlink &&
            echo "g" >> ${TARGETNAME}.jlink &&
            echo "q" >> ${TARGETNAME}.jlink)

    add_custom_target(${TARGETNAME}.flash
        COMMAND JLinkExe ${TARGETNAME}.jlink
        )

    add_dependencies(${TARGETNAME}.flash ${TARGETNAME}.bin)
    add_dependencies(${TARGETNAME}.flash ${TARGETNAME}.jlink)
endfunction()

add_custom_target(
    erase.jlink
    COMMAND
        echo "${SELECT_JLINK_DEVICE}" > erase.jlink &&
        echo "halt" >> erase.jlink &&
        echo "exec device = ${DEFAULT_JLINK_DEVICE_NAME}" >> erase.jlink  &&
        echo "w4 4001E504, 2" >> erase.jlink  &&
        echo "w4 4001e50C, 1" >> erase.jlink  &&
        echo "Sleep 100" >> erase.jlink  &&
        echo "r" >> erase.jlink  &&
        echo "q" >> erase.jlink)

add_custom_target(erase.flash
   COMMAND JLinkExe erase.jlink)

add_dependencies(erase.flash erase.jlink)

add_custom_target(
    softdevice.jlink
    COMMAND
        echo "${SELECT_JLINK_DEVICE}" > softdevice.jlink &&
        echo "halt" >> softdevice.jlink &&
        echo "exec device = ${DEFAULT_JLINK_DEVICE_NAME}" >> softdevice.jlink  &&
        echo "speed 3000" >> softdevice.jlink &&
        echo "loadbin softdevice.bin, 0x00000000" >> softdevice.jlink  &&
        echo "r" >> softdevice.jlink  &&
        echo "g" >> softdevice.jlink  &&
        echo "q" >> softdevice.jlink)

add_custom_target(softdevice.bin
   COMMAND arm-none-eabi-objcopy --input-target ihex --output-target binary ${NRF_SOFTDEVICE_HEX_FILE} softdevice.bin)

add_custom_target(softdevice.flash
    COMMAND JLinkExe softdevice.jlink)

add_dependencies(softdevice.flash softdevice.jlink)
add_dependencies(softdevice.jlink softdevice.bin)

new_executable(beacon)

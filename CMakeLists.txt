# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(PICO2_MCU_PROGRAMMER C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(PICO2_MCU_PROGRAMMER
    src/PICO2_MCU_PROGRAMMER.c
    src/HCS08_BDM.c
    src/PIC_ICSP.c
    src/XMEGA_PDI.c
    src/CYPRESS_HCIUART.c
)

pico_set_program_name(PICO2_MCU_PROGRAMMER "PICO2_MCU_PROGRAMMER")
pico_set_program_version(PICO2_MCU_PROGRAMMER "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(PICO2_MCU_PROGRAMMER 0)
pico_enable_stdio_usb(PICO2_MCU_PROGRAMMER 1)

# Generate the PIO headers from pio/S08_SEND_BYTE.pio, pio/S08_READ_BYTE.pio,
# pio/MCHP_DELAY_NS.pio and pio/XMEGA_PDI.pio. Each generated <basename>.pio.h
# lands in OUTPUT_DIR, which is added to the target's include path, so src/*.c
# include them by bare filename.
# MCHP_DELAY_NS.pio.h also carries the whole PIO_DELAY module (see its c-sdk block).
set(PIO_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/pio)

pico_generate_pio_header(PICO2_MCU_PROGRAMMER ${CMAKE_CURRENT_LIST_DIR}/pio/S08_SEND_BYTE.pio OUTPUT_DIR ${PIO_GENERATED_DIR})
pico_generate_pio_header(PICO2_MCU_PROGRAMMER ${CMAKE_CURRENT_LIST_DIR}/pio/S08_READ_BYTE.pio OUTPUT_DIR ${PIO_GENERATED_DIR})
pico_generate_pio_header(PICO2_MCU_PROGRAMMER ${CMAKE_CURRENT_LIST_DIR}/pio/MCHP_DELAY_NS.pio OUTPUT_DIR ${PIO_GENERATED_DIR})
pico_generate_pio_header(PICO2_MCU_PROGRAMMER ${CMAKE_CURRENT_LIST_DIR}/pio/XMEGA_PDI.pio OUTPUT_DIR ${PIO_GENERATED_DIR})

# Add the standard library to the build
target_link_libraries(PICO2_MCU_PROGRAMMER
    pico_stdlib
    hardware_pio
    hardware_uart
)

# Add the standard include files to the build
target_include_directories(PICO2_MCU_PROGRAMMER PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# CYBT/AIROC bridge: don't gate the CDC link on DTR (the host only drives baud
# changes during the HCI session) and buffer a whole HCI packet in each direction
target_compile_definitions(PICO2_MCU_PROGRAMMER PRIVATE
    PICO_STDIO_USB_CONNECTION_WITHOUT_DTR=1
    CFG_TUD_CDC_RX_BUFSIZE=1024
    CFG_TUD_CDC_TX_BUFSIZE=1024
)

pico_add_extra_outputs(PICO2_MCU_PROGRAMMER)

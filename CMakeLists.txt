set(CMAKE_SYSTEM_NAME       Generic)
set(CMAKE_SYSTEM_VERSION         1 )
cmake_minimum_required(VERSION 3.20)

set(TOOLCHAIN_PATH         B:/arm-gnu-toolchain ) #! toolchain location

# specify cross-compilers and tools
set(CMAKE_C_COMPILER       arm-none-eabi-gcc    )
set(CMAKE_CXX_COMPILER     arm-none-eabi-g++    )
set(CMAKE_ASM_COMPILER     arm-none-eabi-gcc    )
set(CMAKE_AR               arm-none-eabi-ar     )
set(CMAKE_OBJCOPY          arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP          arm-none-eabi-objdump)
set(SIZE                   arm-none-eabi-size   )

set(CMAKE_TRY_COMPILE_TARGET_TYPE   STATIC_LIBRARY)
set(CMAKE_EXPORT_COMPILE_COMMANDS   on)

# project settings
project(stboot    C ASM)
set(CMAKE_C_STANDARD 11)

add_compile_options(
        -mcpu=cortex-m7 -mthumb -mthumb-interwork
        -ffunction-sections -fdata-sections -fno-common -fmessage-length=0
        --include-directory=${TOOLCHAIN_PATH}/arm-none-eabi/include
        # assembler files preprocessing
        $<$<COMPILE_LANGUAGE:ASM>:-x$<SEMICOLON>assembler-with-cpp>
        # optimization level
        # -Og -g
        -O3
)

include_directories(
# libs
        core/include
        drivers/STM32H7xx_HAL_Driver/Inc
        drivers/STM32H7xx_HAL_Driver/Inc/Legacy
        drivers/CMSIS/Device/ST/STM32H7xx/Include
        drivers/CMSIS/Include
        src/lib
# usr
        src
        src/include
)

add_definitions(-DUSE_HAL_DRIVER -DSTM32H743xx) #! H743

file(GLOB_RECURSE SOURCES
        "core/*.*"
        "drivers/*.*"
        "src/*.*"
        "test/*.*"
)

set(LINKER_SCRIPT
        ${CMAKE_SOURCE_DIR}/linker.ld
)

add_link_options(
        -Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map
        -mcpu=cortex-m7 -mthumb -mthumb-interwork
        -T ${LINKER_SCRIPT}
)

add_executable(
        ${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT}
)

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(
        TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex   $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMAND ${SIZE} ${PROJECT_NAME}.elf
        COMMAND powershell -Command "(Get-Item stboot.bin).length / 1KB"
        COMMENT "Building ${HEX_FILE}
        Building ${BIN_FILE}"
)

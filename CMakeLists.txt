cmake_minimum_required(VERSION 3.13.1)

set(MODEL_FOLDER .)
set(EI_SDK_FOLDER edge-impulse-sdk)

include(pico_sdk_import.cmake)

project(pico_standalone C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

pico_sdk_init()

add_executable(pico_standalone
  source/main.cpp
  )

include(${MODEL_FOLDER}/edge-impulse-sdk/cmake/utils.cmake)

target_link_libraries(pico_standalone
                      pico_stdlib
                      hardware_gpio
                      hardware_adc
                      hardware_i2c)

# enable usb output, disable uart output
pico_enable_stdio_usb(pico_standalone 1)
pico_enable_stdio_uart(pico_standalone 0)

target_include_directories(pico_standalone PRIVATE
  ${MODEL_FOLDER}
  ${MODEL_FOLDER}/classifer
  ${MODEL_FOLDER}/tflite-model
  ${MODEL_FOLDER}/model-parameters
  )

target_include_directories(pico_standalone PRIVATE
  ${EI_SDK_FOLDER}
  ${EI_SDK_FOLDER}/third_party/ruy
  ${EI_SDK_FOLDER}/third_party/gemmlowp
  ${EI_SDK_FOLDER}/third_party/flatbuffers/include
  ${EI_SDK_FOLDER}/third_party
  ${EI_SDK_FOLDER}/tensorflow
  ${EI_SDK_FOLDER}/dsp
  ${EI_SDK_FOLDER}/classifier
  ${EI_SDK_FOLDER}/anomaly
  ${EI_SDK_FOLDER}/CMSIS/NN/Include
  ${EI_SDK_FOLDER}/CMSIS/DSP/PrivateInclude
  ${EI_SDK_FOLDER}/CMSIS/DSP/Include
  ${EI_SDK_FOLDER}/CMSIS/Core/Include
  ThirdParty/rp2040_DHT11_lib 
  )

include_directories(${INCLUDES})

# find model source files
RECURSIVE_FIND_FILE(MODEL_FILES "${MODEL_FOLDER}/tflite-model" "*.cpp")
RECURSIVE_FIND_FILE(SOURCE_FILES "${EI_SDK_FOLDER}" "*.cpp")
RECURSIVE_FIND_FILE(CC_FILES "${EI_SDK_FOLDER}" "*.cc")
RECURSIVE_FIND_FILE(S_FILES "${EI_SDK_FOLDER}" "*.s")
RECURSIVE_FIND_FILE(C_FILES "${EI_SDK_FOLDER}" "*.c")
RECURSIVE_FIND_FILE(THIRDPARTY_FILES "ThirdParty" "*.cpp")

list(APPEND SOURCE_FILES ${S_FILES})
list(APPEND SOURCE_FILES ${C_FILES})
list(APPEND SOURCE_FILES ${CC_FILES})
list(APPEND SOURCE_FILES ${MODEL_FILES})
list(APPEND SOURCE_FILES ${THIRDPARTY_FILES})

# add all sources to the project
target_sources(pico_standalone PRIVATE ${SOURCE_FILES})

pico_add_extra_outputs(pico_standalone)

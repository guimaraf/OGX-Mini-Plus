# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "F:/GitRevised/OGX-Mini-Plus/Firmware/external/pico-sdk/tools/pioasm")
  file(MAKE_DIRECTORY "F:/GitRevised/OGX-Mini-Plus/Firmware/external/pico-sdk/tools/pioasm")
endif()
file(MAKE_DIRECTORY
  "F:/GitRevised/OGX-Mini-Plus/Firmware/RP2040/build-debug/pioasm"
  "F:/GitRevised/OGX-Mini-Plus/Firmware/RP2040/build-debug/pioasm-install"
  "F:/GitRevised/OGX-Mini-Plus/Firmware/RP2040/build-debug/pico-sdk/src/rp2_common/tinyusb/pioasm/tmp"
  "F:/GitRevised/OGX-Mini-Plus/Firmware/RP2040/build-debug/pico-sdk/src/rp2_common/tinyusb/pioasm/src/pioasmBuild-stamp"
  "F:/GitRevised/OGX-Mini-Plus/Firmware/RP2040/build-debug/pico-sdk/src/rp2_common/tinyusb/pioasm/src"
  "F:/GitRevised/OGX-Mini-Plus/Firmware/RP2040/build-debug/pico-sdk/src/rp2_common/tinyusb/pioasm/src/pioasmBuild-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "F:/GitRevised/OGX-Mini-Plus/Firmware/RP2040/build-debug/pico-sdk/src/rp2_common/tinyusb/pioasm/src/pioasmBuild-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "F:/GitRevised/OGX-Mini-Plus/Firmware/RP2040/build-debug/pico-sdk/src/rp2_common/tinyusb/pioasm/src/pioasmBuild-stamp${cfgdir}") # cfgdir has leading slash
endif()

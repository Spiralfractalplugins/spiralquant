# CMake toolchain for cross-building Windows targets with MSVC under Wine
# (toolchain installed by msvc-wine into /opt/msvc).
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

set(MSVC_ARCH x64)
set(MSVC_BIN_DIR "/opt/msvc/bin/${MSVC_ARCH}")

set(CMAKE_C_COMPILER   "${MSVC_BIN_DIR}/cl")
set(CMAKE_CXX_COMPILER "${MSVC_BIN_DIR}/cl")
set(CMAKE_RC_COMPILER  "${MSVC_BIN_DIR}/rc")
set(CMAKE_LINKER       "${MSVC_BIN_DIR}/link")
set(CMAKE_AR           "${MSVC_BIN_DIR}/lib")
set(CMAKE_MT           "${MSVC_BIN_DIR}/mt")
set(CMAKE_MIDL         "${MSVC_BIN_DIR}/midl")

# Build-time tools produced by the target toolchain (e.g. the VST3 manifest
# helper) have to run through wine.
set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_CROSSCOMPILING_EMULATOR /usr/local/bin/wine64)

set(CMAKE_FIND_ROOT_PATH /opt/msvc)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

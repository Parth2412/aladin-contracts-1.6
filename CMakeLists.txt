cmake_minimum_required(VERSION 3.5)

project(alaio_contracts)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 6)
set(VERSION_PATCH 1)
#set(VERSION_SUFFIX rc3)

if (VERSION_SUFFIX)
   set(VERSION_FULL "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${VERSION_SUFFIX}")
else()
   set(VERSION_FULL "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
endif()

include(ExternalProject)

find_package(alaio.cdt)

message(STATUS "Building alaio.contracts v${VERSION_FULL}")

set(ALAIO_CDT_VERSION_MIN "1.5")
set(ALAIO_CDT_VERSION_SOFT_MAX "1.5")
#set(ALAIO_CDT_VERSION_HARD_MAX "")

### Check the version of alaio.cdt
set(VERSION_MATCH_ERROR_MSG "")
ALAIO_CHECK_VERSION(VERSION_OUTPUT "${ALAIO_CDT_VERSION}"
                                   "${ALAIO_CDT_VERSION_MIN}"
                                   "${ALAIO_CDT_VERSION_SOFT_MAX}"
                                   "${ALAIO_CDT_VERSION_HARD_MAX}"
                                   VERSION_MATCH_ERROR_MSG)
if(VERSION_OUTPUT STREQUAL "MATCH")
   message(STATUS "Using alaio.cdt version ${ALAIO_CDT_VERSION}")
elseif(VERSION_OUTPUT STREQUAL "WARN")
   message(WARNING "Using alaio.cdt version ${ALAIO_CDT_VERSION} even though it exceeds the maximum supported version of ${ALAIO_CDT_VERSION_SOFT_MAX}; continuing with configuration, however build may fail.\nIt is recommended to use alaio.cdt version ${ALAIO_CDT_VERSION_SOFT_MAX}.x")
else() # INVALID OR MISMATCH
   message(FATAL_ERROR "Found alaio.cdt version ${ALAIO_CDT_VERSION} but it does not satisfy version requirements: ${VERSION_MATCH_ERROR_MSG}\nPlease use alaio.cdt version ${ALAIO_CDT_VERSION_SOFT_MAX}.x")
endif(VERSION_OUTPUT STREQUAL "MATCH")

ExternalProject_Add(
   contracts_project
   SOURCE_DIR ${CMAKE_SOURCE_DIR}/contracts
   BINARY_DIR ${CMAKE_BINARY_DIR}/contracts
   CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${ALAIO_CDT_ROOT}/lib/cmake/alaio.cdt/AlaioWasmToolchain.cmake
   UPDATE_COMMAND ""
   PATCH_COMMAND ""
   TEST_COMMAND ""
   INSTALL_COMMAND ""
   BUILD_ALWAYS 1
)

if (APPLE)
   set(OPENSSL_ROOT "/usr/local/opt/openssl")
elseif (UNIX)
   set(OPENSSL_ROOT "/usr/include/openssl")
endif()
set(SECP256K1_ROOT "/usr/local")

if (APPLE)
   set(OPENSSL_ROOT "/usr/local/opt/openssl")
elseif (UNIX)
   set(OPENSSL_ROOT "/usr/include/openssl")
endif()
set(SECP256K1_ROOT "/usr/local")

string(REPLACE ";" "|" TEST_FRAMEWORK_PATH "${CMAKE_FRAMEWORK_PATH}")
string(REPLACE ";" "|" TEST_MODULE_PATH "${CMAKE_MODULE_PATH}")

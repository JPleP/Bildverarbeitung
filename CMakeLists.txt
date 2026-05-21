cmake_minimum_required(VERSION 3.10)

project( BV_EM )


find_package( OpenCV REQUIRED )

find_package(PkgConfig)
pkg_check_modules(LIBCAMERA REQUIRED IMPORTED_TARGET libcamera)
message(STATUS "libcamera library found:")
message(STATUS "    version: ${LIBCAMERA_VERSION}")
message(STATUS "    libraries: ${LIBCAMERA_LINK_LIBRARIES}")
message(STATUS "    include path: ${LIBCAMERA_INCLUDE_DIRS}")

find_package(OpenMP REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

include_directories( ${OpenCV_INCLUDE_DIRS} ${LIBCAMERA_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)


if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib_RPI)
    set(ARCH_DIR arm64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib_PC)
    set(ARCH_DIR x86_64)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}/${ARCH_DIR}/bin)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}/${ARCH_DIR}/lib)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}/${ARCH_DIR}/lib)




add_executable(BV_EM main.cpp NCNN.cpp Camera.cpp)
target_link_libraries( BV_EM ncnn ${OpenCV_LIBS} PkgConfig::LIBCAMERA)

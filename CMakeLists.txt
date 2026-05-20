cmake_minimum_required(VERSION 3.10)

project( BV_EM )


find_package( OpenCV REQUIRED )

find_package(OpenMP REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

include_directories( ${OpenCV_INCLUDE_DIRS} )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)



add_executable(BV_EM main.cpp NCNN.cpp NMS.cpp)
target_link_libraries( BV_EM ncnn ${OpenCV_LIBS} )

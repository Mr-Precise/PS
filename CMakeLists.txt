cmake_minimum_required(VERSION 3.7.2)

if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
    project(PS)
else()
    project(PS C)
endif()

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

if(MINGW)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mconsole -s")
endif(MINGW)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)

find_package(SDL2 REQUIRED)
include(FindMath)

file(GLOB_RECURSE ${PROJECT_NAME}_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.c)

include_directories(${SDL2_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRC})

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} ${MATH_LIBRARIES})
if(WIN32)
    target_link_libraries(${PROJECT_NAME} imm32 winmm version setupapi hid)
endif(WIN32)
if(MINGW)
    target_link_libraries(${PROJECT_NAME} imm32 winmm version setupapi hid samplerate)
endif(MINGW)

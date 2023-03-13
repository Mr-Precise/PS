cmake_minimum_required(VERSION 3.0.2)
project(PS C)

find_package(SDL2 REQUIRED)

if(MINGW OR WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mconsole -s")
endif(MINGW OR WIN32)


file(GLOB_RECURSE ${PROJECT_NAME}_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.c)

include_directories(${SDL2_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRC})

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
if(WIN32)
    target_link_libraries(${PROJECT_NAME} imm32 winmm version setupapi hid)
endif(WIN32)
if(MINGW)
    target_link_libraries(${PROJECT_NAME} imm32 winmm version setupapi hid samplerate)
endif(MINGW)

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} m)

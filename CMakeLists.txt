cmake_minimum_required(VERSION 3.5)

project(vpl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(VPLSOURCE main.cpp window/VPLRender.hpp window/VPLRender.cpp Player.hpp Player.cpp ffmpeg/Decoder.hpp ffmpeg/Decoder.cpp)

add_executable(vpl ${VPLSOURCE})
target_link_libraries(vpl -lGLEW -lglfw -lGL -ldl -lavformat -lavcodec -lavutil -lswscale -lpthread -lportaudio)

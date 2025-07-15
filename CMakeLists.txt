cmake_minimum_required(VERSION 3.16)

project(uTray
    VERSION 1.2
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_SOURCES
    src/main.cpp
    src/utray.rc
)
if(MSVC) 
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
endif()

message("==================")
message("CXX_FLAGS: ${CMAKE_CXX_FLAGS_RELEASE}")
message("LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
message("==================")

add_executable(uTray ${PROJECT_SOURCES})

target_link_libraries(uTray
    user32
    shell32
    gdi32
    dwmapi
    shlwapi
)

set_target_properties(uTray PROPERTIES
    WIN32_EXECUTABLE TRUE
)

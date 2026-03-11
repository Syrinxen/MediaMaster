cmake_minimum_required(VERSION 3.16)

project(MediaMaster VERSION 1.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Multimedia
    MultimediaWidgets
    Concurrent
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/core/mainwindow.cpp
    src/core/MediaScanner.cpp
    src/core/MediaConverter.cpp
)

set(HEADERS
    include/core/mainwindow.h
    include/core/MediaInfo.h
    include/core/MediaScanner.h
    include/core/MediaConverter.h
)

set(UI_FILES
    src/core/mainwindow.ui
)

qt_add_executable(MediaMaster
    main.cpp
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
)

target_link_libraries(MediaMaster PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Concurrent
)

if(WIN32)
    set_target_properties(MediaMaster PROPERTIES WIN32_EXECUTABLE TRUE)

    add_custom_command(TARGET MediaMaster POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ffmpeg/ffmpeg.exe"
        $<TARGET_FILE_DIR:MediaMaster>
    )
endif()
cmake_minimum_required(VERSION 3.5)

project(vp415emu)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Get the Git branch and revision
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_compile_definitions(APP_BRANCH=\"${GIT_BRANCH}\")
add_compile_definitions(APP_COMMIT=\"${GIT_COMMIT_HASH}\")

# Set the target name
set(TARGET_NAME vp415emu)

# Find the Qt library
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
    SerialPort
    Multimedia
    MultimediaWidgets
)

# Add all source files from main directory and subdirectories
file(GLOB_RECURSE SRC_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

file(GLOB UI_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/*.ui
)

# Create the executable target with both source and UI files
add_executable(${TARGET_NAME} 
    ${SRC_FILES}
    ${UI_FILES}
)

# Include directories should include both src, ui and binary dirs
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
)

# Set UI output directory
set_target_properties(${TARGET_NAME} PROPERTIES
    AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

# Link the Qt libraries
target_link_libraries(${TARGET_NAME} PRIVATE
    Qt::Core
    Qt::Widgets
    Qt::SerialPort
    Qt::Multimedia
    Qt::MultimediaWidgets
)

install(TARGETS ${TARGET_NAME})

# FORMS    += mainwindow.ui \
#     settingsdialog.ui \
#     serialmonitordialog.ui \
#     fcodemonitordialog.ui \
#     frameviewerdialog.ui


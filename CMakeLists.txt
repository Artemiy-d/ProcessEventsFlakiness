# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(process_events_flakiness LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(process_events_flakiness
    MANUAL_FINALIZATION
    widget.h widget.cpp
    main.cpp
)

target_link_libraries(process_events_flakiness PUBLIC
    Qt::Widgets
)

set_target_properties(process_events_flakiness PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

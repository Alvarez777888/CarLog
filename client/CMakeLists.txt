cmake_minimum_required(VERSION 3.16)
project(CarLog LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)
set(CARLOG_SOURCES
    main.cpp
    api.cpp
    loginwindow.cpp
    recovery.cpp
    mainwindow.cpp
    navigation.cpp
    home.cpp
    cars_layout.cpp
    cars_view.cpp
    records_layout.cpp
    stats_layout.cpp
    data.cpp
    forms.cpp
    settings.cpp
    account.cpp
    formatting.cpp
    export.cpp
    resources/resources.qrc)
set(CARLOG_API_URL "" CACHE STRING "Published HTTPS API URL for mobile clients")
if(ANDROID OR IOS)
    if(QT_VERSION_MAJOR LESS 6 OR Qt6_VERSION VERSION_LESS "6.11")
        message(FATAL_ERROR "Use Qt 6.11 or newer for the mobile build")
    endif()
    if(NOT CARLOG_API_URL MATCHES "^https://[A-Za-z0-9.-]+(:[0-9]+)?/?$")
        message(FATAL_ERROR "Set CARLOG_API_URL to your deployed HTTPS server URL")
    endif()
    if(CARLOG_API_URL MATCHES "^https://(localhost|127\\.0\\.0\\.1)([:/]|$)")
        message(FATAL_ERROR "Mobile clients require a reachable server, not localhost")
    endif()
    configure_file(mobile-config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/mobile-config.hpp @ONLY)
endif()
if(QT_VERSION_MAJOR EQUAL 6)
    qt_add_executable(carlog MANUAL_FINALIZATION ${CARLOG_SOURCES})
else()
    add_executable(carlog ${CARLOG_SOURCES})
endif()
if(WIN32 AND NOT ANDROID)
    enable_language(RC)
    configure_file(resources/carlog.rc.in ${CMAKE_CURRENT_BINARY_DIR}/carlog.rc @ONLY)
    target_sources(carlog PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/carlog.rc)
endif()
target_include_directories(carlog PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
if(ANDROID)
    set_target_properties(carlog PROPERTIES
        QT_ANDROID_PACKAGE_NAME "org.carlog.app"
        QT_ANDROID_VERSION_NAME "1.0"
        QT_ANDROID_VERSION_CODE 1
        )
    qt_add_android_permission(carlog NAME android.permission.INTERNET)
endif()
if(IOS)
    set_target_properties(carlog PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.carlog.app"
        MACOSX_BUNDLE_BUNDLE_NAME "CarLog"
        MACOSX_BUNDLE_BUNDLE_VERSION "1"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0")
endif()
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_target(carlog)
endif()
target_link_libraries(carlog PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)
if(MSVC)
    target_compile_options(carlog PRIVATE /utf-8)
endif()


option(CARLOG_BUILD_SMOKE "Build the UI check for a disposable test server" OFF)
if(CARLOG_BUILD_SMOKE)
    set(CARLOG_SMOKE_SOURCES ${CARLOG_SOURCES})
    list(REMOVE_ITEM CARLOG_SMOKE_SOURCES main.cpp)
    add_executable(carlog-client-smoke ../tests/client_smoke.cpp ${CARLOG_SMOKE_SOURCES})
    target_include_directories(carlog-client-smoke PRIVATE .)
    if(MSVC)
        target_compile_options(carlog-client-smoke PRIVATE /utf-8)
    endif()
    target_link_libraries(carlog-client-smoke PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)
endif()

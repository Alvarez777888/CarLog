QT += widgets network
CONFIG += c++17
TEMPLATE = app
TARGET = carlog
SOURCES = main.cpp api.cpp loginwindow.cpp recovery.cpp mainwindow.cpp navigation.cpp home.cpp cars_layout.cpp cars_view.cpp records_layout.cpp stats_layout.cpp data.cpp forms.cpp settings.cpp account.cpp formatting.cpp export.cpp
HEADERS = api.hpp loginwindow.hpp mainwindow.hpp pages.hpp formatting.hpp theme.hpp
RESOURCES = resources/resources.qrc
msvc:QMAKE_CXXFLAGS += /utf-8

HEADERS += mobile.hpp

win32:RC_ICONS = resources/carlog.ico

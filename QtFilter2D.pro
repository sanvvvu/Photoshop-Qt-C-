QT += widgets
CONFIG += c++17

TEMPLATE = app
TARGET = QtFilter2D

SOURCES += \
    main.cpp \
    filter.cpp \
    ImageInfoWidget.cpp

HEADERS += \
    filter.h \
    ImageInfoWidget.h

DESTDIR = build
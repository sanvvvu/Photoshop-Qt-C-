QT += widgets concurrent
CONFIG += c++17

TEMPLATE = app
TARGET = QtFilter2D

SOURCES += \
    main.cpp \
    MainWindow.cpp \
    filter.cpp \
    ImageInfoWidget.cpp \
    GrayscaleBT601.cpp \
    GrayscaleBT709.cpp \
    Otsu.cpp \
    Huang.cpp \
    Niblack.cpp \
    ISOMAD.cpp

HEADERS += \
    MainWindow.h \
    filter.h \
    ImageInfoWidget.h \
    GrayscaleBT601.h \
    GrayscaleBT709.h \
    Otsu.h \
    Huang.h \
    Niblack.h \
    ISOMAD.h

DESTDIR = build
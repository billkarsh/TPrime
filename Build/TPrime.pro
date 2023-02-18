
TEMPLATE = app
TARGET   = TPrime

win32 {
    DESTDIR = C:/Users/labadmin/Desktop/SGLBUILD/FIXU/TPrime/TPrime-win
#    DESTDIR = C:/Users/labadmin/Desktop/SGLBUILD/FIXU/TPrime/Debug
}

unix {
    DESTDIR = /home/billkarsh/Code/TPrime/TPrime-linux
}

QT += widgets

HEADERS +=              \
    cnpy.h              \
    CGBL.h              \
    Cmdline.h           \
    Tool.h              \
    Util.h

SOURCES +=              \
    cnpy.cpp            \
    main.cpp            \
    CGBL.cpp            \
    Cmdline.cpp         \
    Tool.cpp            \
    Util.cpp            \
    Util_osdep.cpp

win32 {
    LIBS    += -lWS2_32 -lUser32 -lwinmm
    DEFINES += _CRT_SECURE_NO_WARNINGS WIN32
}

QMAKE_TARGET_COMPANY = Bill Karsh
QMAKE_TARGET_PRODUCT = TPrime
QMAKE_TARGET_DESCRIPTION = Remaps event times
QMAKE_TARGET_COPYRIGHT = (c) 2020, Bill Karsh, All rights reserved
VERSION = 1.9



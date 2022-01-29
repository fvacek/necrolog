message("including $$PWD")

QT -= core gui

CONFIG += C++11
CONFIG += hide_symbols

TEMPLATE = lib
TARGET = necrolog

isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
	SHV_PROJECT_TOP_BUILDDIR=$$shadowed($$PWD)/..
}
message ( SHV_PROJECT_TOP_BUILDDIR: '$$SHV_PROJECT_TOP_BUILDDIR' )

unix:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

message ( DESTDIR: $$DESTDIR )

DEFINES += NECROLOG_BUILD_DLL

INCLUDEPATH += \

LIBS += \
    #-L$$DESTDIR \
    #-lqfcore

SOURCES += \
    necrolog.cpp

HEADERS += \
    necrolog.h \
    necrologglobal.h


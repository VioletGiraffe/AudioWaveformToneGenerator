###################################################
#            Basic configuration
###################################################

TEMPLATE = app
#TARGET   = NewAwesomeApplication

QT = core gui widgets charts
#win*:QT += winextras
#CONFIG -= qt
#CONFIG += console

CONFIG += strict_c++ c++2a

mac* | linux* | freebsd{
	CONFIG(release, debug|release):CONFIG *= Release optimize_full
	CONFIG(debug, debug|release):CONFIG *= Debug
}

contains(QT_ARCH, x86_64) {
	ARCHITECTURE = x64
} else {
	ARCHITECTURE = x86
}

android {
	Release:OUTPUT_DIR=android/release
	Debug:OUTPUT_DIR=android/debug

} else:ios {
	Release:OUTPUT_DIR=ios/release
	Debug:OUTPUT_DIR=ios/debug

} else {
	Release:OUTPUT_DIR=release/$${ARCHITECTURE}
	Debug:OUTPUT_DIR=debug/$${ARCHITECTURE}
}

DESTDIR  = ../bin/$${OUTPUT_DIR}
OBJECTS_DIR = ../build/$${OUTPUT_DIR}/$${TARGET}
MOC_DIR     = ../build/$${OUTPUT_DIR}/$${TARGET}
UI_DIR      = ../build/$${OUTPUT_DIR}/$${TARGET}
RCC_DIR     = ../build/$${OUTPUT_DIR}/$${TARGET}

###################################################
#               INCLUDEPATH
###################################################

INCLUDEPATH += \
	../qtutils \
	../cpputils \
	../cpp-template-utils

###################################################
#                 HEADERS
###################################################

HEADERS += \
	src/audio/caudiooutputwasapi.h \
	src/cmainwindow.h

###################################################
#                 SOURCES
###################################################

SOURCES += \
	src/audio/caudiooutputwasapi.cpp \
	src/cmainwindow.cpp \
	src/main.cpp

###################################################
#                 LIBS
###################################################


LIBS += -L../bin/$${OUTPUT_DIR} -lcpputils

mac*|linux*|freebsd{
	PRE_TARGETDEPS += $${DESTDIR}/libcpputils.a
}

###################################################
#    Platform-specific compiler options and libs
###################################################

win*{
	LIBS += -lole32
	QMAKE_CXXFLAGS += /MP /Zi /FS /wd4251
	QMAKE_CXXFLAGS += /std:c++latest /permissive- /Zc:__cplusplus
	QMAKE_CXXFLAGS_WARN_ON = /W4
	DEFINES += WIN32_LEAN_AND_MEAN NOMINMAX _SCL_SECURE_NO_WARNINGS

	QMAKE_LFLAGS += /DEBUG:FASTLINK

	Debug:QMAKE_LFLAGS += /INCREMENTAL
	Release:QMAKE_LFLAGS += /OPT:REF /OPT:ICF

	INCLUDEPATH += $${PWD}/../wil/include
}

mac*{
	LIBS += -framework AppKit

	#QMAKE_POST_LINK = cp -f -p $$PWD/$$DESTDIR/*.dylib $$PWD/$$DESTDIR/$${TARGET}.app/Contents/MacOS/
}

###################################################
#      Generic stuff for Linux and Mac
###################################################

linux*|mac*|freebsd{
	QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-c++11-extensions -Wno-local-type-template-args -Wno-deprecated-register

	Release:DEFINES += NDEBUG=1
	Debug:DEFINES += _DEBUG
}

FORMS += \
	src/cmainwindow.ui

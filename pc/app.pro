
TARGET = app
TEMPLATE = app
QT += core gui bluetooth widgets serialport

CONFIG += c++17

LIBS += -lws2_32

SOURCES += \
	BluetoothDeviceInfo.cpp \
	osc.cpp \
	main.cpp\
	BLEInterface.cpp \
	BitWidget.cpp \
	MainWindow.cpp \
	sock.cpp

HEADERS  += \
	BitWidget.h \
	BLEInterface.h \
	BitWidget.h \
	BluetoothDeviceInfo.h \
	MainWindow.h \
	osc.h \
	jstream.h \
	sock.h

FORMS    += \
	MainWindow.ui

RESOURCES += \
	resources/resources.qrc

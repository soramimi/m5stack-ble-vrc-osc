#include "BluetoothDeviceInfo.h"
#include <QBluetoothAddress>
#include <QBluetoothDeviceInfo>

BluetoothDeviceInfo::BluetoothDeviceInfo(const QBluetoothDeviceInfo &device)
	: QObject()
	, device_(device)
{
}

void BluetoothDeviceInfo::setDevice(const QBluetoothDeviceInfo &device)
{
	device_ = device;
	emit deviceChanged();
}

QString BluetoothDeviceInfo::getName() const
{
	return device_.name();
}

QString BluetoothDeviceInfo::getAddress() const
{
#ifdef Q_OS_MAC
	// workaround for Core Bluetooth:
	return m_device.deviceUuid().toString();
#else
	return device_.address().toString();
#endif
}

QBluetoothDeviceInfo BluetoothDeviceInfo::getDevice() const
{
	return device_;
}

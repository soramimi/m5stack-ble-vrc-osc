#ifndef BLUETOOTHDEVICEINFO_H
#define BLUETOOTHDEVICEINFO_H

#include <QBluetoothDeviceInfo>
#include <QObject>

class BluetoothDeviceInfo: public QObject {
	Q_OBJECT
private:
	QBluetoothDeviceInfo device_;
public:
	BluetoothDeviceInfo(const QBluetoothDeviceInfo &device);
	void setDevice(const QBluetoothDeviceInfo &device);
	QString getName() const;
	QString getAddress() const;
	QBluetoothDeviceInfo getDevice() const;
signals:
	void deviceChanged();

};


#endif // BLUETOOTHDEVICEINFO_H

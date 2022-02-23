#ifndef BLEINTERFACE_H
#define BLEINTERFACE_H

#include "BluetoothDeviceInfo.h"

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <vector>

const int CHUNK_SIZE  = 20;

typedef QList<std::shared_ptr<BluetoothDeviceInfo>> BluetoothDeviceInfoPtr;

class BLEInterface : public QObject {
	Q_OBJECT
private:
	struct Private;
	Private *m;
public:
	BluetoothDeviceInfoPtr devices() const;
private:
	void searchCharacteristic();
	inline void waitForWrite();
	void updateConnected(bool connected);
	void clearService();
public:
	int getCurrentDevice() const;
	void setCurrentDevice(int index);
public:
	explicit BLEInterface();
	~BLEInterface();

	void connectCurrentDevice();
	void disconnectDevice();
	void clearDevices();
	void scanDevices();
	void write(const QByteArray &data);

	bool isConnected() const;

	int currentService() const;

	QStringList services() const;
public slots:
	void setCurrentService(int currentService);
private slots:
	//QBluetothDeviceDiscoveryAgent
	void addDevice(const QBluetoothDeviceInfo&);
	void onScanFinished();
	void onDeviceScanError(QBluetoothDeviceDiscoveryAgent::Error);

	//QLowEnergyController
	void onServiceDiscovered(const QBluetoothUuid &);
	void onServiceScanDone();
	void onControllerError(QLowEnergyController::Error);
	void onDeviceConnected();
	void onDeviceDisconnected();

	//QLowEnergyService
	void onServiceStateChanged(QLowEnergyService::ServiceState s);
	void onCharacteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value);
	void serviceError(QLowEnergyService::ServiceError e);

	void read();
	void onCharacteristicRead(const QLowEnergyCharacteristic &c, const QByteArray &value);
	void onCharacteristicWrite(const QLowEnergyCharacteristic &c, const QByteArray &value);
	void updateCurrentService(int index);
signals:
	void devicesChanged();
	void servicesChanged();
	void statusInfoChanged(QString info, bool isGood);
	void dataReceived(const QByteArray &data, const QLowEnergyCharacteristic &c);
	void connectionChanged(bool connected);

	void currentServiceChanged(int currentService);
};

#endif // BLEINTERFACE_H

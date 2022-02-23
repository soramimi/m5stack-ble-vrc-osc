#include "BLEInterface.h"
#include <QDebug>
#include <QEventLoop>
#include <memory>

struct BLEInterface::Private {
	int current_device_index = -1;
	QStringList devices_names;
	QStringList services;
	std::shared_ptr<QBluetoothDeviceDiscoveryAgent> device_discovery_agent;

	QLowEnergyDescriptor notification_desc;
	std::shared_ptr<QLowEnergyController> control;
	QList<QBluetoothUuid> services_uuid;
	QLowEnergyService *service = nullptr;
	QLowEnergyCharacteristic read_characteristic;
	QLowEnergyCharacteristic write_characteristic;
	BluetoothDeviceInfoPtr devices;

	bool connected = false;
	int current_service = 0;
	QLowEnergyService::WriteMode write_mode = QLowEnergyService::WriteWithResponse;
};

BLEInterface::BLEInterface()
	: m(new Private)
{
	m->device_discovery_agent = std::make_shared<QBluetoothDeviceDiscoveryAgent>();
	connect(m->device_discovery_agent.get(), SIGNAL(deviceDiscovered(const QBluetoothDeviceInfo&)), this, SLOT(addDevice(const QBluetoothDeviceInfo&)));
	connect(m->device_discovery_agent.get(), SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)), this, SLOT(onDeviceScanError(QBluetoothDeviceDiscoveryAgent::Error)));
	connect(m->device_discovery_agent.get(), SIGNAL(finished()), this, SLOT(onScanFinished()));
}

BLEInterface::~BLEInterface()
{
	disconnectDevice();
	delete m;
}

void BLEInterface::clearService()
{
	m->current_service = 0;
	delete m->service;
	m->service = nullptr;
}

void BLEInterface::clearDevices()
{
	m->connected = false;
	m->current_device_index = -1;
	clearService();
	m->devices_names.clear();
	m->devices.clear();
}

void BLEInterface::disconnectDevice()
{
	if (m->control) {
		m->control->disconnectFromDevice();
	}
	clearDevices();
}

void BLEInterface::scanDevices()
{
	disconnectDevice();
	emit devicesChanged();
	m->device_discovery_agent->stop();
	m->device_discovery_agent->start();
	emit statusInfoChanged("Scanning for devices...", true);
}

void BLEInterface::read()
{
	if (m->service && m->read_characteristic.isValid()) {
		m->service->readCharacteristic(m->read_characteristic);
	}
}

void BLEInterface::write(const QByteArray &data)
{
//	qDebug() << "BLEInterface::write: " << data;
	if (m->service && m->write_characteristic.isValid()) {
		if (data.length() > CHUNK_SIZE) {
			int sentBytes = 0;
			while (sentBytes < data.length()) {
				m->service->writeCharacteristic(m->write_characteristic, data.mid(sentBytes, CHUNK_SIZE), m->write_mode);
				sentBytes += CHUNK_SIZE;
//				if (m_writeMode == QLowEnergyService::WriteWithResponse) {
//					waitForWrite();
//					if (m_service->error() != QLowEnergyService::NoError)
//						return;
//				}
			}
		} else {
			m->service->writeCharacteristic(m->write_characteristic, data, m->write_mode);
		}
	}
}

bool BLEInterface::isConnected() const
{
	return m->connected;
}

int BLEInterface::currentService() const
{
	return m->current_service;
}

void BLEInterface::setCurrentService(int index)
{
	if (m->current_service == index) return;
	updateCurrentService(index);
	m->current_service = index;
	emit currentServiceChanged(index);
}

void BLEInterface::addDevice(const QBluetoothDeviceInfo &device)
{
	if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
//		qWarning() << "Discovered LE Device name: " << device.name() << " Address: " << device.address().toString();
		m->devices_names.append(device.name());
		std::shared_ptr<BluetoothDeviceInfo> dev = std::make_shared<BluetoothDeviceInfo>(device);
		m->devices.push_back(dev);
		emit devicesChanged();
		emit statusInfoChanged("Low Energy device found. Scanning for more...", true);
	}
	//...
}

void BLEInterface::onScanFinished()
{
	if (m->devices_names.size() == 0) {
		emit statusInfoChanged("No Low Energy devices found", false);
	}
}

void BLEInterface::onDeviceScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
	if (error == QBluetoothDeviceDiscoveryAgent::PoweredOffError) {
		emit statusInfoChanged("The Bluetooth adaptor is powered off, power it on before doing discovery.", false);
	} else if (error == QBluetoothDeviceDiscoveryAgent::InputOutputError) {
		emit statusInfoChanged("Writing or reading from the device resulted in an error.", false);
	} else {
		emit statusInfoChanged("An unknown error has occurred.", false);
	}
}



void BLEInterface::connectCurrentDevice()
{
	if (m->devices.empty()) return;

	if (m->control) {
		m->control->disconnectFromDevice();
		m->control.reset();
	}
	if (m->current_device_index < 0 || m->current_device_index >= m->devices.size()) {
		return;
	}
	m->control = std::make_shared<QLowEnergyController>(m->devices[m->current_device_index]->getDevice());
	connect(m->control.get(), SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(onServiceDiscovered(QBluetoothUuid)));
	connect(m->control.get(), SIGNAL(discoveryFinished()), this, SLOT(onServiceScanDone()));
	connect(m->control.get(), SIGNAL(error(QLowEnergyController::Error)), this, SLOT(onControllerError(QLowEnergyController::Error)));
	connect(m->control.get(), SIGNAL(connected()), this, SLOT(onDeviceConnected()));
	connect(m->control.get(), SIGNAL(disconnected()), this, SLOT(onDeviceDisconnected()));
	m->control->connectToDevice();
}

void BLEInterface::onDeviceConnected()
{
	m->services_uuid.clear();
	m->services.clear();
	setCurrentService(-1);
	emit servicesChanged();
	m->control->discoverServices();
}

void BLEInterface::onDeviceDisconnected()
{
	m->services_uuid.clear();
	m->services.clear();
	setCurrentService(-1);
	updateConnected(false);
	emit statusInfoChanged("Service disconnected", false);
//	qWarning() << "Remote device disconnected";
}

void BLEInterface::onServiceDiscovered(const QBluetoothUuid &gatt)
{
	Q_UNUSED(gatt)
	emit statusInfoChanged("Service discovered. Waiting for service scan to be done...", true);
}

void BLEInterface::onServiceScanDone()
{
	m->services_uuid = m->control->services();
	if (m->services_uuid.isEmpty()) {
		emit statusInfoChanged("Can't find any services.", true);
	} else {
		m->services.clear();
		for (auto const &uuid : m->services_uuid) {
			m->services.append(uuid.toString());
		}
		emit servicesChanged();
		m->current_service = -1;// to force call update_currentService(once)
		setCurrentService(0);
		emit statusInfoChanged("All services discovered.", true);
	}
}

void BLEInterface::onControllerError(QLowEnergyController::Error error)
{
	emit statusInfoChanged("Cannot connect to remote device.", false);
//	qWarning() << "Controller Error:" << error;
}



void BLEInterface::onCharacteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
	emit dataReceived(value, c);
}

void BLEInterface::onCharacteristicWrite(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
//	qDebug() << "Characteristic Written: " << value;
}

void BLEInterface::updateCurrentService(int index)
{
	if (index >= 0 && m->services_uuid.count() > index) {
		m->service = m->control->createServiceObject(m->services_uuid.at(index), this);
	}

	if (!m->service) {
		emit statusInfoChanged("Service not found.", false);
		return;
	}

	connect(m->service, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(onServiceStateChanged(QLowEnergyService::ServiceState)));
	connect(m->service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this, SLOT(onCharacteristicChanged(QLowEnergyCharacteristic,QByteArray)));
	connect(m->service, SIGNAL(characteristicRead(QLowEnergyCharacteristic,QByteArray)),  this, SLOT(onCharacteristicRead(QLowEnergyCharacteristic,QByteArray)));
	connect(m->service, SIGNAL(characteristicWritten(QLowEnergyCharacteristic,QByteArray)), this, SLOT(onCharacteristicWrite(QLowEnergyCharacteristic,QByteArray)));
	connect(m->service, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(serviceError(QLowEnergyService::ServiceError)));

	if (m->service->state() == QLowEnergyService::DiscoveryRequired) {
		emit statusInfoChanged("Connecting to service...", true);
		m->service->discoverDetails();
	} else {
		searchCharacteristic();
	}
}
void BLEInterface::onCharacteristicRead(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
//	qDebug() << "Characteristic Read: " << value;
	emit dataReceived(value, c);
}



BluetoothDeviceInfoPtr BLEInterface::devices() const
{
	return m->devices;
}

QStringList BLEInterface::services() const
{
	return m->services;
}

void BLEInterface::searchCharacteristic()
{
	if (m->service) {
		for (QLowEnergyCharacteristic const &c : m->service->characteristics()) {
			if (c.isValid()) {
				if (c.properties() & QLowEnergyCharacteristic::WriteNoResponse || c.properties() & QLowEnergyCharacteristic::Write) {
					m->write_characteristic = c;
					updateConnected(true);
					if (c.properties() & QLowEnergyCharacteristic::WriteNoResponse) {
						m->write_mode = QLowEnergyService::WriteWithoutResponse;
					} else {
						m->write_mode = QLowEnergyService::WriteWithResponse;
					}
				}
				if (c.properties() & QLowEnergyCharacteristic::Read) {
					m->read_characteristic = c;
				}
				m->notification_desc = c.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
				if (m->notification_desc.isValid()) {
					m->service->writeDescriptor(m->notification_desc, QByteArray::fromHex("0100"));
				}
			}
		}
	}
}

void BLEInterface::updateConnected(bool connected)
{
	if (connected != m->connected) {
		m->connected = connected;
		emit connectionChanged(connected);
	}
}

int BLEInterface::getCurrentDevice() const
{
	return m->current_device_index;
}

void BLEInterface::setCurrentDevice(int index)
{
	if (m->current_device_index != index) {
		m->current_device_index = index;
	}
}

void BLEInterface::onServiceStateChanged(QLowEnergyService::ServiceState s)
{
//	qDebug() << "serviceStateChanged, state: " << s;
	if (s == QLowEnergyService::ServiceDiscovered) {
		searchCharacteristic();
	}
}

void BLEInterface::serviceError(QLowEnergyService::ServiceError e)
{
//	qWarning() << "Service error:" << e;
}

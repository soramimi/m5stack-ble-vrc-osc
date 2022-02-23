
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QDateTime>
#include <QStatusBar>
#include <QThread>
#include "osc.h"
#include "jstream.h"

#if 1
char const *targetDeviceName()
{
	return "M5Stack";
}

char const *targetServiceUUID()
{
	return "{5147b804-4b5b-429d-b6d2-0f4b8187a4ea}";
}

char const *targetCharacteristicUUID()
{
	return "{a851d6b3-6720-41e7-a9d4-81dcec2fd861}";
}
#else
char const *targetDeviceName()
{
	return "M5StickC";
}

char const *targetServiceUUID()
{
	return "{1ed42829-cd7a-44e6-87db-24fed6422bc4}";
}

char const *targetCharacteristicUUID()
{
	return "{503423d1-ad1f-4ed3-b3df-06e276798fba}";
}
#endif

class CustomEvent : public QEvent {
public:
	enum Type {
		DeviceChanged = QEvent::User,
		ServicesChanged,
	};
	CustomEvent(Type type)
		: QEvent((QEvent::Type)type)
	{
	}
};

enum ConnectionFlag {
	DeviceAvailable = 0x01,
	ServiceAvailable = 0x02,
	ConnectionReady = 0x04,
};

struct MainWindow::Private {
	std::shared_ptr<BLEInterface> ble_interface;
	int connection_flags = 0;
	bool closing = false;

	int buttons = 0;

	osc::Transmitter osc_tx;
};

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m(new Private)
{
	ui->setupUi(this);

	m->ble_interface = std::make_shared<BLEInterface>();
	connect(m->ble_interface.get(), &BLEInterface::dataReceived, this, &MainWindow::dataReceived);
	connect(m->ble_interface.get(), &BLEInterface::devicesChanged, this, [&](){
		QApplication::postEvent(this, new CustomEvent(CustomEvent::DeviceChanged));
	});
	connect(m->ble_interface.get(), &BLEInterface::servicesChanged, this, [&](){
		QApplication::postEvent(this, new CustomEvent(CustomEvent::ServicesChanged));
	});
	connect(m->ble_interface.get(), &BLEInterface::statusInfoChanged, [this](QString info, bool good) {
		showStatusMessage(info);
	});
	connect(m->ble_interface.get(), &BLEInterface::connectionChanged, this, &MainWindow::connectionChanged);

	scanDevices();

	m->osc_tx.open("127.0.0.1");
}

MainWindow::~MainWindow()
{
	m->closing = true;
	m->osc_tx.close();
	delete m;
	delete ui;
}

void MainWindow::customEvent(QEvent *event)
{
	switch ((CustomEvent::Type)event->type()) {
	case CustomEvent::DeviceChanged:
		devicesChanged();
		return;
	case CustomEvent::ServicesChanged:
		servicesChanged();
		return;
	}
}

void MainWindow::scanDevices()
{
	m->connection_flags = 0;
	ui->devicesComboBox->clear();
	ui->servicesComboBox->clear();
	m->ble_interface->scanDevices();
}

void MainWindow::showStatusMessage(QString text)
{
	if (m->connection_flags & ConnectionReady) {
		text = "Ready";
	}
	statusBar()->showMessage(text);
}

void MainWindow::connectionChanged(bool connected)
{
	if (connected) {
		m->connection_flags |= ConnectionReady;
	} else {
		m->connection_flags = 0;
		if (!m->closing) {
			scanDevices();
		}
	}
	showStatusMessage({});
}

void MainWindow::devicesChanged()
{
	if (m->connection_flags & DeviceAvailable) return;

	bool b = ui->devicesComboBox->blockSignals(true);
	ui->devicesComboBox->clear();

	int sel = -1;
	QString device_name = targetDeviceName();
	auto devs = m->ble_interface->devices();
	for (int i = 0; i < devs.size(); i++) {
		QString name = devs[i]->getName();
		ui->devicesComboBox->addItem(name);
		if (name == device_name) {
			sel = i;
		}
	}
	if (sel >= 0) {
		m->connection_flags = DeviceAvailable;
		m->ble_interface->setCurrentDevice(sel);
		m->ble_interface->connectCurrentDevice();
		ui->devicesComboBox->setCurrentIndex(sel);
	}

	ui->devicesComboBox->blockSignals(b);
}

void MainWindow::servicesChanged()
{
	if (m->connection_flags & ServiceAvailable) return;

	bool b = ui->servicesComboBox->blockSignals(true);
	ui->servicesComboBox->clear();

	int sel = -1;
	QString service_uuid = targetServiceUUID();
	auto servs = m->ble_interface->services();
	for (int i = 0; i < servs.size(); i++) {
		QString name = servs[i];
		ui->servicesComboBox->addItem(name);
		if (name.compare(service_uuid, Qt::CaseInsensitive) == 0) {
			sel = i;
		}
	}
	if (sel >= 0) {
		m->connection_flags |= ServiceAvailable;
		m->ble_interface->setCurrentService(sel);
		ui->servicesComboBox->setCurrentIndex(sel);
	}

	ui->servicesComboBox->blockSignals(b);
}

void MainWindow::connectToDevice()
{
	m->ble_interface->setCurrentDevice(ui->devicesComboBox->currentIndex());
	m->ble_interface->connectCurrentDevice();
}

void MainWindow::on_servicesComboBox_currentIndexChanged(int index)
{
	m->ble_interface->setCurrentService(index);
}

void MainWindow::buttonChanged(uint8_t diff, uint8_t bits)
{
	if (diff & 1) {
		m->osc_tx.send_int("/input/Jump", bits & 1);
	}
}

void MainWindow::setButtons(uint8_t v)
{
	buttonChanged(m->buttons ^ v, v);

	ui->widget_bit0->setValue((v >> 0) & 1);
	ui->widget_bit1->setValue((v >> 1) & 1);
	ui->widget_bit2->setValue((v >> 2) & 1);
	ui->widget_bit3->setValue((v >> 3) & 1);
	ui->widget_bit4->setValue((v >> 4) & 1);
	ui->widget_bit5->setValue((v >> 5) & 1);
	ui->widget_bit6->setValue((v >> 6) & 1);
	ui->widget_bit7->setValue((v >> 7) & 1);

	m->buttons = v;
}

void MainWindow::dataReceived(QByteArray data, const QLowEnergyCharacteristic &c)
{
	QString uuid = targetCharacteristicUUID();
	if (uuid.compare(c.uuid().toString(), Qt::CaseInsensitive) == 0) {
		if (data.size() >= 1) {
			int v = data[0];
			setButtons(v);
		}
	}
}



void MainWindow::on_action_connect_triggered()
{
	scanDevices();
}

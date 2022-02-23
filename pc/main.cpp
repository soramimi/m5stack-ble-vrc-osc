#include "MainWindow.h"
#include <QApplication>
#include "sock.h"

int main(int argc, char **argv)
{
	sock::startup();

	QApplication a(argc, argv);
	MainWindow w;
	w.show();
	auto r = a.exec();

	sock::cleanup();
	return r;
}

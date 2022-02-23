#ifndef BITWIDGET_H
#define BITWIDGET_H

#include <QWidget>

class BitWidget : public QWidget {
	Q_OBJECT
private:
	bool value_ = false;
protected:
	void paintEvent(QPaintEvent *event);
public:
	explicit BitWidget(QWidget *parent = nullptr);
	void setValue(bool v);
};

#endif // BITWIDGET_H

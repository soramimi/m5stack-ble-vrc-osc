#include "BitWidget.h"

#include <QPainter>


BitWidget::BitWidget(QWidget *parent)
	: QWidget(parent)
{

}

void BitWidget::setValue(bool v)
{
	value_ = v;
	update();
}

void BitWidget::paintEvent(QPaintEvent *event)
{
	QRect r = rect();
	QPainter pr(this);
	pr.fillRect(r, Qt::black);
	r.adjust(1, 1, -1, -1);
	pr.fillRect(r, value_ ? QColor(0, 255, 0) : QColor(0, 64, 0));
}

#pragma once

#include <QTimer>
#include "ui_ztoolwidget.h"

#define ZTOOLLABEL_HEIGHT	320
#define ZTOOLLABEL_WIDTH	240

class ZToolWidget : public QWidget
{
	Q_OBJECT

	static ZToolWidget *pZToolWidget;

	void closeEvent(QCloseEvent *)
	{
		pZToolWidget = NULL;
	}
public:
	ZToolWidget(QWidget *parent = 0, Qt::WindowFlags flags = 0)
	{
		ui.setupUi(this);
		setAttribute(Qt::WA_DeleteOnClose);
		setWindowFlags(Qt::ToolTip);
		setFixedSize(ZTOOLLABEL_WIDTH, ZTOOLLABEL_HEIGHT);
	}
	~ZToolWidget()
	{
		delete pixmap;
		pixmap = NULL;
	}
	static bool Show(const QPoint &p)
	{
		QPoint pos = p;

		if(pZToolWidget)
			return false;
//			delete pZToolWidget;
		pZToolWidget = NULL;

		if(!pixmap || pixmap->isNull())
			return false;
	
		pZToolWidget = new ZToolWidget;

		int w = ZTOOLLABEL_WIDTH, h = ZTOOLLABEL_HEIGHT;
		QSize s = pixmap->size();
		if(s.width() > s.height())
		{
			w = ZTOOLLABEL_HEIGHT;
			h = ZTOOLLABEL_WIDTH;
			pZToolWidget->setFixedSize(w, h);
		}

		if(pos.y() > h)
			pos.setY(pos.y() - h);
		if(pos.x() > w)
			pos.setX(pos.x() - w);
		pZToolWidget->move(pos);

		pZToolWidget->ui.label->setPixmap(*pixmap);
		pZToolWidget->show();
		QTimer::singleShot(1500, pZToolWidget, SLOT(close()));
		return true;
	}
	static QPixmap *pixmap;
	Ui::ToolLabel ui;
};

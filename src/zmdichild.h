#pragma once

#include <QDialog>
#include <QWidget>
#include "zview.h"

class ZMdiChild : public QDialog
{
    Q_OBJECT

public:
    ZMdiChild(QWidget* parent, Qt::WindowFlags flags);
 
	void setContextMenuForTbl(const QStringList &items);
	virtual void init(const QString &){};
	// set icon and title
	virtual void setWindowTitleAndIcon(const QString &title, const QIcon &icon) 
	{
		setWindowTitle(title);
		setWindowIcon(icon);
	}
	virtual void execCustomAction(const QString &){};
	void reload() {if(m_tbl) m_tbl->reload();}
protected:
    void closeEvent(QCloseEvent *event);
	ZView *m_tbl;

signals:
	void needUpdate();
	void needUpdateVal(int);
protected slots:
	void slotCustomActionExec();
};



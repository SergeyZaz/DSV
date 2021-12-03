#ifndef ZMDICHILD_H
#define ZMDICHILD_H

#include <QDialog>
#include <QSqlDatabase>
#include "zview.h"

class ZMdiChild : public QDialog
{
    Q_OBJECT

public:
    ZMdiChild();
 
	void setContextMenuForTbl(const QStringList &items);
	virtual void initDB(QSqlDatabase &, const QString &){};
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

#endif

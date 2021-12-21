#pragma once
#include <QWidget>
#include <QSqlQueryModel>
#include <QSortFilterProxyModel>
#include "ui_ztariffs.h"

class ZTariffs : public QWidget
{
	Q_OBJECT

	Ui::ZTariffs ui;

	static QStringList l_Modes;

	QSqlQueryModel* model;
	QSortFilterProxyModel sortModel;
	int currentId;
	void Update();
	int openEditor(int id = -1);

public:
	ZTariffs(QWidget* parent, Qt::WindowFlags flags = 0);
 
	void init();

	static QStringList& getModes();
public slots:
	void setCurrentElem(QEvent::Type, int id);
	void add();
	void del();
	void edit();
	void doubleClickedTbl(const QModelIndex&);

};



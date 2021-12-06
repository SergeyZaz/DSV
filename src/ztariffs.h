#pragma once
#include <QDialog>
#include <QSqlDatabase>
#include <QSqlQueryModel>
#include <QSortFilterProxyModel>
#include "ui_ztariffs.h"

class ZTariffs : public QDialog
{
	Q_OBJECT

	Ui::ZTariffs ui;

	QSqlDatabase	m_DB;
	QSqlQueryModel* model;
	QSortFilterProxyModel sortModel;
	int currentId;
	void Update();
	int openEditor(int id = -1);

public:
	ZTariffs(QWidget* parent, Qt::WindowFlags flags = 0);
 
	void initDB(QSqlDatabase &m_DB);

public slots:
	void setCurrentElem(QEvent::Type, int id);
	void add();
	void del();
	void edit();

};



#pragma once

#include "ui_znotebooksform.h"
#include "zeditbaseform.h"
#include "zmdichild.h"


class ZNotebooksForm : public ZEditAbstractForm
{
	Q_OBJECT

public:
	ZNotebooksForm(QWidget* parent, Qt::WindowFlags flags = 0);
	~ZNotebooksForm();

	int init(const QString& table, int id);
	Ui::ZNotebooksForm ui;

protected slots:
	void applyChanges();
	void addNewSlot();
};


class ZNotebooks : public ZMdiChild
{

public:
	ZNotebooks(QWidget* parent, Qt::WindowFlags flags = 0);
 
	void init(const QString &m_TblName);
};



#pragma once

#include "ui_zusersform.h"
#include "zeditbaseform.h"
#include "zmdichild.h"

bool CheckPwd(const QString& login, const QString& psw, int* pType = NULL);

class ZUsersForm : public ZEditAbstractForm
{
	Q_OBJECT

public:
	ZUsersForm(QWidget* parent, Qt::WindowFlags flags = 0);
	~ZUsersForm();

	int init(const QString& table, int id);
	Ui::ZUsersForm ui;

protected slots:
	void applyChanges();
	void addNewSlot();
};


class ZUsers : public ZMdiChild
{

public:
	ZUsers(QWidget* parent, Qt::WindowFlags flags = 0);
 
	void init(const QString &m_TblName);
};



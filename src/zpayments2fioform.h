#pragma once

#include "ui_zpayments2fioform.h"
#include "zeditbaseform.h"

class ZPayments2FioForm : public ZEditAbstractForm
{
    Q_OBJECT
	Ui::ZPayments2FioForm ui;

	void loadFio();

public:
	ZPayments2FioForm(QWidget* parent, Qt::WindowFlags flags = 0);
	~ZPayments2FioForm();

	int init(const QString& table, int id);

protected slots:
	void applyChanges();
	void changeMode(int indx);


};


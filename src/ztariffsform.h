#pragma once

#include "ui_ztariffsform.h"
#include "zeditbaseform.h"

class ZTariffsForm : public ZEditAbstractForm
{
    Q_OBJECT
	Ui::ZTariffsForm ui;

public:
	ZTariffsForm(QWidget* parent, Qt::WindowFlags flags = 0);
	~ZTariffsForm();

	int init(const QString &table, int id );

protected slots:
	void applyChanges();

};


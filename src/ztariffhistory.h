#pragma once

#include "ui_ztariffhistory.h"
#include <QDialog>

class ZTariffsHistory : public QDialog
{
    Q_OBJECT
	Ui::ZTariffHistoryForm ui;

public:
	ZTariffsHistory(QWidget* parent, Qt::WindowFlags flags = 0);
	~ZTariffsHistory();

	int getData(QDate &d, double &val);

protected slots:
	void applyChanges();

};


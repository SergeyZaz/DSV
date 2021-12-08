#pragma once

#include "ui_zpaymentsform.h"
#include "zeditbaseform.h"

class ZPaymentsForm : public ZEditAbstractForm
{
    Q_OBJECT
	Ui::ZPaymentsForm ui;

public:
	ZPaymentsForm(QWidget* parent, Qt::WindowFlags flags = 0);
	~ZPaymentsForm();

	int init( QSqlDatabase &database, const QString &table, int id );

protected slots:
	void applyChanges();

};


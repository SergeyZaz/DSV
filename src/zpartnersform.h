#pragma once

#include "ui_zpartnersform.h"
#include "zeditbaseform.h"

class ZPartnersForm : public ZEditAbstractForm
{
    Q_OBJECT
	Ui::ZPartnersForm ui;

public:
    ZPartnersForm();
	~ZPartnersForm();

	int init( QSqlDatabase &database, const QString &table, int id );
	void showEvent(QShowEvent*);

protected slots:
	void applyChanges();
	void addAddSection();

};


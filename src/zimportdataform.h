#pragma once

#include "ui_zimportdataform.h"
#include "zeditbaseform.h"

class ZImportDataForm : public ZEditAbstractForm
{
    Q_OBJECT

	void loadCbo(QComboBox* cbo, QString tbl);

public:
	ZImportDataForm(QWidget* parent, Qt::WindowFlags flags = 0);
	~ZImportDataForm();

	int init(const QString& table, int id);
	Ui::ZImportDataForm ui;

protected slots:
	void applyChanges();
	void addNewSlot();
};


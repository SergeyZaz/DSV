#pragma once

#include "ui_znotesform.h"
#include "zeditbaseform.h"

class ZNotesForm : public ZEditAbstractForm
{
    Q_OBJECT

	void loadFio();

public:
	ZNotesForm(QWidget* parent, Qt::WindowFlags flags = 0);
	~ZNotesForm();

	int init(const QString& table, int id);
	Ui::ZNotesForm ui;

protected slots:
	void applyChanges();
	void addNewSlot();
};


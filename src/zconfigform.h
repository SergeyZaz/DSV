#pragma once

#include <QDialog>
#include "ui_zconfigform.h"

bool GetCloseDate();

class ZConfigForm : public QDialog
{
	Q_OBJECT

public:
	ZConfigForm(QWidget* parent, Qt::WindowFlags flags = 0);
	~ZConfigForm();

	Ui::ZConfigForm ui;

protected slots:
	void applyChanges();
};


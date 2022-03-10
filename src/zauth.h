#pragma once

#include <QDialog>
#include "ui_zauth.h"

class ZAuthForm : public QDialog
{
	Q_OBJECT

public:
	ZAuthForm(QWidget* parent, Qt::WindowFlags flags = 0);
	~ZAuthForm();

	Ui::ZAuthForm ui;

	int execute();

protected slots:
	void applyChanges();
};


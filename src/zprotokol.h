#pragma once
#include <QDialog>
#include "ui_zprotokol.h"

class ZProtokol : public QDialog
{
	Q_OBJECT

	//void loadItemsToListBox( QListWidget *listBox, const QString &tableName);

public:
	ZProtokol(QWidget* parent, Qt::WindowFlags flags = 0);
	~ZProtokol();

public slots:
	void buildProtokol();
	void saveProtokol();

private:
	Ui::ZProtokol ui;
};


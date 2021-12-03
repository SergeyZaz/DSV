#ifndef ZPROTOKOL_H
#define ZPROTOKOL_H

#include <QDialog>
#include <QSqlDatabase>
#include "ui_zprotokol.h"

class ZProtokol : public QDialog
{
	Q_OBJECT

	QSqlDatabase	db;
	void loadItemsToListBox( QListWidget *listBox, const QString &tableName);
	double getSumma(QTreeWidgetItem *pItem, int col);
	int saveToFile(const QString &fileName);
	int saveToXLSFile(const QString &fileName);

public:
	ZProtokol(QSqlDatabase &m_db, QWidget *parent = 0);
	~ZProtokol();

public slots:
	void buildProtokol();
	void saveProtokol();
	void openFilterProtokol(QTreeWidgetItem *item, int column);

private:
	Ui::ZProtokol ui;
};

#endif // ZPROTOKOL_H

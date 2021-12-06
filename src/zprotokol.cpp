#include <QSqlQuery>
#include <QSqlError>
#include "zprotokol.h"
#include "zmessager.h"

ZProtokol::ZProtokol(QWidget* parent, Qt::WindowFlags flags): QDialog(parent, flags)
{
	ui.setupUi(this);
	//setAttribute(Qt::WA_DeleteOnClose);

	ui.dateStart->setDate(QDate::currentDate().addMonths(-1));
	ui.dateEnd->setDate(QDate::currentDate());
	
	connect(ui.cmdBuild, SIGNAL(clicked()), this, SLOT(buildProtokol()));
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(saveProtokol()));	

	buildProtokol();
}

ZProtokol::~ZProtokol()
{

}
/*
void ZProtokol::loadItemsToListBox( QListWidget *listBox, const QString &tableName)
{
	QSqlQuery query;

	// clear box
	listBox->clear();
	auto result = query.exec(QString("SELECT id, name FROM %1 ORDER BY name").arg(tableName));
	if (result)
	{
		while (query.next()) 
		{
			QListWidgetItem *pItem = new QListWidgetItem(query.value(1).toString().simplified());
			pItem->setData(Qt::UserRole, query.value(0).toInt());
			pItem->setCheckState(Qt::Checked);
			listBox->addItem(pItem);
		}
	}
}
*/
void ZProtokol::saveProtokol()
{
}

void ZProtokol::buildProtokol()
{
	QString stringQuery = QString("SELECT dt, import_data.fio, fio.name, organisation2fio.key, organisation.name, smena, tariff, num  FROM import_data \
INNER JOIN fio ON(import_data.fio = fio.id)\
INNER JOIN organisation2fio ON(import_data.fio = value)\
INNER JOIN organisation ON(organisation2fio.key = organisation.id) WHERE dt >= '%1' AND dt <= '%2' ORDER BY dt")
		.arg(ui.dateStart->date().toString("yyyy-MM-dd"))
		.arg(ui.dateEnd->date().toString("yyyy-MM-dd"));

	QSqlQuery query;
	if (!query.exec(stringQuery))
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
		return;
	}

	ui.tree->clear();

	QTreeWidgetItem  *pItem, *pItemGroup;
	QMap<int, QTreeWidgetItem*> mapFIO;
	int id;
	double v;

	while (query.next())
	{
		id = query.value(1).toInt();
		pItemGroup = mapFIO.value(id);
		if (!pItemGroup)
		{
			pItemGroup = new QTreeWidgetItem(ui.tree);
			pItemGroup->setText(0, query.value(4).toString());
			pItemGroup->setText(1, query.value(2).toString());
			ui.tree->addTopLevelItem(pItemGroup);
			mapFIO.insert(id, pItemGroup);
		}

		pItem = new QTreeWidgetItem(pItemGroup);
		pItem->setText(0, query.value(0).toDate().toString("yyyy-MM-dd"));
		pItem->setText(1, QString::number(query.value(5).toInt()));
		pItem->setText(2, QString::number(query.value(6).toInt()));
		v = query.value(7).toDouble();
#ifndef MONEY_FORMAT
		pItem->setText(3, QString::number(v, 'f', 2));
#else
		pItem->setText(3, QString("%L1").arg(v, 0, 'f', 2));
#endif
	}
}
	

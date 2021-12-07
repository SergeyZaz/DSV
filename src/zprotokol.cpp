#include <QSqlQuery>
#include <QSqlError>
#include "zprotokol.h"
#include "zmessager.h"
#include "ztariffs.h"

ZProtokol::ZProtokol(QWidget* parent, Qt::WindowFlags flags): QDialog(parent, flags)
{
	ui.setupUi(this);
	//setAttribute(Qt::WA_DeleteOnClose);

	ui.dateStart->setDate(QDate::currentDate().addMonths(-1));
	ui.dateEnd->setDate(QDate::currentDate());
	
	connect(ui.cmdBuild, SIGNAL(clicked()), this, SLOT(buildProtokol()));
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(saveProtokol()));	

	ui.tree->setColumnWidth(0, 200);
	ui.tree->setColumnWidth(1, 200);
	ui.tree->setColumnWidth(2, 200);
	ui.tree->setColumnWidth(3, 200);

	loadItemsTo(ui.cboFilter, "groups");
	buildProtokol();
}

ZProtokol::~ZProtokol()
{

}

void ZProtokol::loadTariffs()
{
	QSqlQuery query, query2;
	auto result = query.exec(QString("SELECT id, txt, mode, bonus, type FROM tariff ORDER BY id"));
	if (!result)
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
		return;
	}

	l_tariffs.clear();
	
	QStringList l_T = ZTariffs::getModes();
	QString txt;

	while (query.next())
	{
			tariff t;
			t.id = query.value(0).toInt();
			t.mode = query.value(2).toInt(); //0-столбец "станок" (вид работы), 1-столбец "модель шины" содержит txt, 2-столбец "модель шины" начинается с txt, 3-столбец "модель шины" заканчивается txt, 4-столбец "Rate" значение txt
			if (t.mode < l_T.size())
			{
				txt = l_T[t.mode];
				t.name = query.value(1).toString();
				if (txt.contains("..."))
					t.name = txt.replace("...", t.name);
			}
			t.type = query.value(4).toInt();// плата: 0-за смену, 1-за штуку
			t.bonus = query.value(3).toDouble();

			if (!query2.exec(QString("SELECT dt, val FROM tariff_history WHERE tariff_id=%1 ORDER BY dt").arg(t.id)))
				continue;

			while (query2.next())
			{
				tariff_history t_h;
				t_h.date = query2.value(0).toDate();
				t_h.val = query2.value(1).toDouble();

				t.l_history.push_back(t_h);
			}

			l_tariffs.push_back(t);
	}
}

void ZProtokol::loadItemsTo(QComboBox* cbo, const QString &tableName)
{
	QSqlQuery query;
	cbo->clear();
	auto result = query.exec(QString("SELECT id, name FROM %1 ORDER BY id").arg(tableName));
	if (result)
	{
		while (query.next()) 
		{
			cbo->addItem(query.value(1).toString(), query.value(0).toInt());
		}
	}
	cbo->setCurrentIndex(0);
}

double ZProtokol::getTariffValue(const QDate &date, int id, int num, QString &txt, double &bonus)
{
	double v = 0;

	bonus = 0;

	foreach(tariff t, l_tariffs)
	{
		if (t.id != id)
			continue;
		txt = t.name;
		bonus = t.bonus;

		foreach(tariff_history tH, t.l_history)
		{
			if (tH.date > date)
				break;
			v = tH.val;
		}
		if (t.type == 1) //за штуку
			v *= num;
		v += t.bonus;
		break;
	}
	return v;
}

double ZProtokol::getSumma(QTreeWidgetItem* pItemRoot, int col)
{
	double s = 0;

	if (!pItemRoot)
		return s;
	
	int n = pItemRoot->childCount();
	if (n > 0)
	{
		for (int i = 0; i < n; i++)
		{
			s += getSumma(pItemRoot->child(i), col);
		}
	}
	else
	{
		QString txt = pItemRoot->text(col).replace(QChar::Nbsp, "");
		bool ok;
		double v = txt.toDouble(&ok);
		if (!ok)
		{
			txt.replace(",", ".");
			v = txt.toDouble(&ok);
		}
		s += v;
	}
	return s;
}

void ZProtokol::buildProtokol()
{
	loadTariffs();

	QString stringQuery = QString("SELECT dt, import_data.fio, fio.name, organisation.id, organisation.name, smena, smena.name, tariff, num, groups.id, fio.comment  FROM import_data \
INNER JOIN fio ON(import_data.fio = fio.id) \
INNER JOIN organisation2fio ON(import_data.fio = value) \
INNER JOIN organisation ON(organisation2fio.key = organisation.id) \
INNER JOIN smena ON(smena.id = smena) \
INNER JOIN groups2fio ON(import_data.fio = groups2fio.value) \
INNER JOIN groups ON(groups2fio.key = groups.id) \
WHERE dt >= '%1' AND dt <= '%2' ORDER BY dt")
.arg(ui.dateStart->date().toString("yyyy-MM-dd"))
.arg(ui.dateEnd->date().toString("yyyy-MM-dd"));

	QSqlQuery query;
	if (!query.exec(stringQuery))
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
		return;
	}

	ui.tree->clear();

	QTreeWidgetItem* pItem, * pItemGroup;
	QMap<int, QTreeWidgetItem*> mapFIO;
	int id, num;
	double v, bonus;
	QString txt;
	QDate date;

	int groupId = ui.cboFilter->currentData().toInt();

	while (query.next())
	{
		if (groupId != 0 && groupId != query.value(9).toInt())
			continue;

		id = query.value(1).toInt();
		pItemGroup = mapFIO.value(id);
		if (!pItemGroup)
		{
			pItemGroup = new QTreeWidgetItem(ui.tree);
			pItemGroup->setText(0, query.value(4).toString());
			pItemGroup->setText(1, query.value(2).toString());
			pItemGroup->setText(6, query.value(10).toString());
			ui.tree->addTopLevelItem(pItemGroup);
			mapFIO.insert(id, pItemGroup);
		}

		pItem = new QTreeWidgetItem(pItemGroup);

		date = query.value(0).toDate();
		pItem->setText(0, date.toString("yyyy-MM-dd"));
		pItem->setText(1, query.value(6).toString());

		id = query.value(7).toInt();
		num = query.value(8).toDouble();

		pItem->setText(2, QString::number(num));

		v = getTariffValue(date, id, num, txt, bonus);
		pItem->setText(3, txt);
#ifndef MONEY_FORMAT
		pItem->setText(5, QString::number(v, 'f', 2));
		//		pItem->setText(4, QString::number(bonus, 'f', 2));
#else
		pItem->setText(5, QString("%L1").arg(v, 0, 'f', 2));
		//		pItem->setText(4, QString("%L1").arg(bonus, 0, 'f', 2));
#endif
	}

	QFont fnt;
	int i, j, n = ui.tree->topLevelItemCount();
	for (j = 0; j < n; j++)
	{
		pItem = ui.tree->topLevelItem(j);

		v = getSumma(pItem, 5);
#ifndef MONEY_FORMAT
		pItem->setText(5, QString::number(v, 'f', 2));
#else
		pItem->setText(5, QString("%L1").arg(v, 0, 'f', 2));
#endif

		for (i = 0; i < pItem->columnCount(); i++)
		{
			fnt = pItem->font(i);
			fnt.setBold(true);
			pItem->setFont(i, fnt);
		}
	}
}

void ZProtokol::saveProtokol()
{
}


#include <QSqlQuery>
#include <QSqlError>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include "zprotokol.h"
#include "zmessager.h"
#include "ztariffs.h"
#include "zpayments2fioform.h"

#include "xlsxdocument.h"
using namespace QXlsx;

#define FIO_ID_ROLE			Qt::UserRole
#define PAYMENT_ID_ROLE		Qt::UserRole+1
#define PAYMENT_ROLE		Qt::UserRole+2
#define COUNT_SMENS_ROLE	Qt::UserRole+3
#define BONUS_ROLE			Qt::UserRole+4
#define TARIFF_ROLE			Qt::UserRole+5
#define COUNT_ROLE			Qt::UserRole+6

int Round(double dVal)
{
	return ((int)dVal / 100 + ((int)dVal % 100 >= 50 ? 1 : 0)) * 100;
//	if ((dVal - (int)dVal) >= 0.5)
//		return (int)ceil(dVal);
//	else
//		return (int)floor(dVal);
}

ZProtokol::ZProtokol(QWidget* parent, Qt::WindowFlags flags)//: QDialog(parent, flags)
{
	ui.setupUi(this);
	//setAttribute(Qt::WA_DeleteOnClose);

	ui.dateStart->setDate(QDate::currentDate().addMonths(-1));
	ui.dateEnd->setDate(QDate::currentDate());

	connect(ui.cmdBuild, SIGNAL(clicked()), this, SLOT(buildProtokol()));
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(saveProtokol()));
	connect(ui.ckbExpandAll, SIGNAL(clicked(bool)), this, SLOT(expandAll(bool)));

	ui.lblSumma->setVisible(false);

	ui.tree->setColumnWidth(0, 200);
	ui.tree->setColumnWidth(1, 200);
	ui.tree->setColumnWidth(2, 250);
	ui.tree->setColumnWidth(3, 250);
	ui.tree->setColumnWidth(4, 300);
	ui.tree->setColumnWidth(5, 200);

	ui.tree->setItemDelegate(new ZTreeDataDelegate(this, ui.tree));

	loadItemsToComboBox(ui.cboFilter, "groups");
	buildProtokol();
}


ZProtokol::~ZProtokol()
{

}

QSize ZProtokol::sizeHint() const
{
	return QSize(1700, 750);
}

void ZProtokol::expandAll(bool fCheck)
{
	if (fCheck)
		ui.tree->expandAll();
	else
		ui.tree->collapseAll();
}

void ZProtokol::loadTariffs()
{
	QSqlQuery query, query2;
	auto result = query.exec(QString("SELECT id, txt, mode, bonus, type FROM tariff ORDER BY pr"));
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

			//0 - станок ..., 1 - станок начинается с ..., 2 - станок содержит ..., 3 - станок заканчивается ...,
			//4 - модель шины ..., 5 - модель шины начинается с ..., 6 - модель шины содержит ..., 7 - модель шины заканчивается ...,
			//8 - Rate ..., 9 - Rate начинается с ..., 10 - Rate содержит ..., 11 - Rate заканчивается ...		
			t.mode = query.value(2).toInt();
			if (t.mode < l_T.size())
			{
				txt = l_T[t.mode];
				t.name = query.value(1).toString();
				if (txt.contains("..."))
					t.name = txt.replace("...", "\"" + t.name + "\"");
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

double ZProtokol::getTariffValue(const QDate &date, int id, int num, QString &txt, double& bonus, double& t_val, bool &fSmena)
{
	double v = 0;

	t_val = 0;
	bonus = 0;
	fSmena = true;

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

		t_val = v;

		if (t.type == 1) //за штуку
		{
			fSmena = false;
			v *= num;
		}

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
		s += QString2Double(pItemRoot->text(col));
	}
	return s;
}

void ZProtokol::buildProtokol()
{
	loadTariffs();

	QString dateStartStr = ui.dateStart->date().toString(DATE_FORMAT);
	QString dateEndStr = ui.dateEnd->date().toString(DATE_FORMAT);

	QString stringQuery = QString("SELECT dt, import_data.fio, fio.name, organisation.id, organisation.name, smena, smena.name, tariff, num, groups.id, fio.comment  FROM import_data \
INNER JOIN fio ON(import_data.fio = fio.id) \
LEFT JOIN organisation2fio ON(import_data.fio = value) \
LEFT JOIN organisation ON(organisation2fio.key = organisation.id) \
INNER JOIN smena ON(smena.id = smena) \
LEFT JOIN groups2fio ON(import_data.fio = groups2fio.value) \
LEFT JOIN groups ON(groups2fio.key = groups.id) \
WHERE dt >= '%1' AND dt <= '%2' ORDER BY fio.name,dt")
.arg(dateStartStr)
.arg(dateEndStr);

	QSqlQuery query;
	if (!query.exec(stringQuery))
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
		return;
	}

	ui.tree->clear();
	ui.ckbExpandAll->setCheckState(Qt::Unchecked);

	QTreeWidgetItem *pItem, *pItemGroup;
	QMap<int, QTreeWidgetItem*> mapFIO;
	int id, num;
	double v, bonus, tV;
	QString txt;
	QDate date;
	int i, j, n;
	QVariantList vList;
	QStringList sList;
	bool isSmena;

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
			pItemGroup->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);

			pItemGroup->setText(0, query.value(4).toString());
			pItemGroup->setText(1, query.value(2).toString());
			pItemGroup->setData(1, FIO_ID_ROLE, id);
			pItemGroup->setText(6, query.value(10).toString());
			
			ui.tree->addTopLevelItem(pItemGroup);
			mapFIO.insert(id, pItemGroup);

			for (i = 3; i < 5; i++)
			{
				pItemGroup->setData(i, FIO_ID_ROLE, id);

				getTextForPayment(id, i, txt, vList, v);

				pItemGroup->setText(i, txt);
				pItemGroup->setData(i, PAYMENT_ID_ROLE, vList);

				pItemGroup->setData(i, PAYMENT_ROLE, v);
			}
			pItemGroup->setData(6, FIO_ID_ROLE, id);

			pItemGroup->setSizeHint(0, QSize(100, 50));

			QString stringQuery2 = QString("SELECT note FROM notes2fio WHERE fio = %1 AND ((begin_dt >= '%2' AND begin_dt <= '%3') OR (end_dt >= '%2' AND end_dt <= '%3'))")
				.arg(id).arg(dateStartStr).arg(dateEndStr);
			QSqlQuery query2;
			if (query2.exec(stringQuery2))
			{
				stringQuery2.clear();

				while (query2.next())
				{
					stringQuery2 += query2.value(0).toString() + "\n";
				}
				stringQuery2.chop(1);
				pItemGroup->setText(7, stringQuery2);
			}
		}

		pItem = new QTreeWidgetItem(pItemGroup);

		date = query.value(0).toDate();
		pItem->setText(0, date.toString(DATE_FORMAT));
		pItem->setText(1, query.value(6).toString());

		id = query.value(7).toInt();
		num = query.value(8).toDouble();

		v = getTariffValue(date, id, num, txt, bonus, tV, isSmena);
		pItem->setData(0, BONUS_ROLE, bonus);
		pItem->setData(0, TARIFF_ROLE, tV);
		pItem->setData(0, COUNT_ROLE, isSmena ? 1 : num);

		// если в названии тарифа есть слово "смена" то считаю количество - числом смен
		pItem->setData(0, COUNT_SMENS_ROLE, txt.contains("смена") ? num : 1);

		pItem->setText(3, txt);
		
#ifndef MONEY_FORMAT
		pItem->setText(2, QString::number(v, 'f', 2));
#else
		pItem->setText(2, QString("%L1").arg(v, 0, 'f', 2));
#endif
	}

	//пересчитываю доплаты, которые начислились в один день (заменяю на средние значения)
	n = ui.tree->topLevelItemCount();
	for (j = 0; j < n - 1; j++)
	{
		pItem = ui.tree->topLevelItem(j);
		if (!pItem)
			continue;
		//QString sTxt = pItem->text(1);
		int childs = pItem->childCount();
		for (i = 0; i < childs; i++)
		{
			QList<QTreeWidgetItem*> l_Itms;
			QTreeWidgetItem *pIt1 = pItem->child(i);
			for (int ii = i + 1; ii < childs; ii++)
			{
				QTreeWidgetItem* pIt2 = pItem->child(ii);
				if (pIt1->text(0) == pIt2->text(0) && pIt1->text(1) == pIt2->text(1))
					l_Itms << pIt2;
			}

			int s = l_Itms.size();
			if (s == 0)
				continue;
			
			i += s;
			
			int nn = l_Itms[0]->data(0, COUNT_SMENS_ROLE).toInt();
			l_Itms[0]->setData(0, COUNT_SMENS_ROLE, QString::number(nn - s));

			l_Itms << pIt1;
			s++;

			double s_d = 0;
			foreach(pIt1, l_Itms)
				s_d += pIt1->data(0, BONUS_ROLE).toDouble();
			s_d /= s * s; //среднее на строку
			foreach(pIt1, l_Itms)
			{
				//QString sTxt1 = pIt1->text(0);
				v = QString2Double(pIt1->text(2)) - pIt1->data(0, BONUS_ROLE).toDouble() + s_d;
#ifndef MONEY_FORMAT
				pIt1->setText(2, QString::number(v, 'f', 2));
#else
				pIt1->setText(2, QString("%L1").arg(v, 0, 'f', 2));
#endif
				pIt1->setData(0, BONUS_ROLE, s_d);
			}
		}
	}

	//добавляю формулу расчета начислений
	for (j = 0; j < n - 1; j++)
	{
		pItem = ui.tree->topLevelItem(j);
		if (!pItem)
			continue;
		int childs = pItem->childCount();
		for (i = 0; i < childs; i++)
		{
			QTreeWidgetItem* pIt1 = pItem->child(i);

			bonus = pIt1->data(0, BONUS_ROLE).toDouble();
			num = pIt1->data(0, COUNT_ROLE).toInt();
			tV = pIt1->data(0, TARIFF_ROLE).toDouble();
			pIt1->setText(4, QString("(%1 * %2) + %3").arg(tV, 0, 'f', 2).arg(num).arg(bonus, 0, 'f', 2));
		}
	}

	for (int zz = 0; zz < 2; zz++)
	{
		//добавляю ФИО для которых нет смен но есть выплаты!
		stringQuery = (zz == 0) ? 

			QString("SELECT payments2fio.fio, fio.name, organisation.id, organisation.name, groups.id, fio.comment FROM payments2fio \
INNER JOIN fio ON(payments2fio.fio = fio.id) \
LEFT JOIN organisation2fio ON(payments2fio.fio = value) \
LEFT JOIN organisation ON(organisation2fio.key = organisation.id) \
LEFT JOIN groups2fio ON(payments2fio.fio = groups2fio.value) \
LEFT JOIN groups ON(groups2fio.key = groups.id) \
WHERE dt_link >= '2022-01-16' AND dt_link <= '2022-02-16' ORDER BY fio.name")
.arg(dateStartStr)
.arg(dateEndStr)

		:

			//добавляю ФИО для которых нет смен но есть заметки!
			QString("SELECT notes2fio.fio, fio.name, organisation.id, organisation.name, groups.id, fio.comment FROM notes2fio \
INNER JOIN fio ON(notes2fio.fio = fio.id) \
LEFT JOIN organisation2fio ON(notes2fio.fio = value) \
LEFT JOIN organisation ON(organisation2fio.key = organisation.id) \
LEFT JOIN groups2fio ON(notes2fio.fio = groups2fio.value) \
LEFT JOIN groups ON(groups2fio.key = groups.id) \
WHERE((begin_dt >= '%1' AND begin_dt <= '%2') OR(end_dt >= '%1' AND end_dt <= '%2')) AND char_length(note) > 0 ORDER BY fio.name")
.arg(dateStartStr)
.arg(dateEndStr);

		if (!query.exec(stringQuery))
		{
			ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
			return;
		}
		while (query.next())
		{
			if (groupId != 0 && groupId != query.value(4).toInt())
				continue;

			id = query.value(0).toInt();
			pItemGroup = mapFIO.value(id);
			if (!pItemGroup)
			{
				pItemGroup = new QTreeWidgetItem(ui.tree);
				pItemGroup->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);

				pItemGroup->setText(0, query.value(3).toString());
				pItemGroup->setText(1, query.value(1).toString());
				pItemGroup->setData(1, FIO_ID_ROLE, id);
				pItemGroup->setText(6, query.value(5).toString());

				ui.tree->addTopLevelItem(pItemGroup);
				mapFIO.insert(id, pItemGroup);

				for (i = 3; i < 5; i++)
				{
					pItemGroup->setData(i, FIO_ID_ROLE, id);

					getTextForPayment(id, i, txt, vList, v);

					pItemGroup->setText(i, txt);
					pItemGroup->setData(i, PAYMENT_ID_ROLE, vList);

					pItemGroup->setData(i, PAYMENT_ROLE, v);
				}
				pItemGroup->setData(6, FIO_ID_ROLE, id);

				pItemGroup->setSizeHint(0, QSize(100, 50));

				QString stringQuery2 = QString("SELECT note FROM notes2fio WHERE fio = %1 AND ((begin_dt >= '%2' AND begin_dt <= '%3') OR (end_dt >= '%2' AND end_dt <= '%3'))")
					.arg(id).arg(dateStartStr).arg(dateEndStr);
				QSqlQuery query2;
				if (query2.exec(stringQuery2))
				{
					stringQuery2.clear();

					while (query2.next())
					{
						stringQuery2 += query2.value(0).toString() + "\n";
					}
					stringQuery2.chop(1);
					pItemGroup->setText(7, stringQuery2);
				}
			}
		}
	}
	
	QFont fnt;
	pItem = new QTreeWidgetItem(ui.tree);
	pItem->setText(0, "Итого:");
	ui.tree->addTopLevelItem(pItem);
	fnt = pItem->font(0);
	fnt.setPointSizeF(14);
	fnt.setBold(true);
	pItem->setFont(0, fnt);

	updateSumm();

	n = ui.tree->topLevelItemCount();
	for (j = 0; j < n - 1; j++)
	{
		pItem = ui.tree->topLevelItem(j);

		for (i = 0; i < pItem->columnCount(); i++)
		{
			fnt = pItem->font(i);
			if (i > 2 && i < 5 || i == 6)
				fnt.setPointSizeF(10);
			else
				fnt.setBold(true);
			pItem->setFont(i, fnt);
		}

		//количество смен
		int smens = 0, childs = pItem->childCount();
		for (i = 0; i < childs; i++)
		{
			pItemGroup = pItem->child(i);
			smens += pItemGroup->data(0, COUNT_SMENS_ROLE).toInt();
		}
		pItem->setText(8, QString::number(smens));

		//среднее за смену
		if (smens > 0)
		{
			v = QString2Double(pItem->text(2)) / smens;
#ifndef MONEY_FORMAT
			pItem->setText(9, QString::number(v, 'f', 2));
#else
			pItem->setText(9, QString("%L1").arg(v, 0, 'f', 2));
#endif
		}
	}

	for(i = 0; i < ui.tree->columnCount(); i++)
		ui.tree->resizeColumnToContents(i);
}

void ZProtokol::roundSumm()
{
	int j, n = ui.tree->topLevelItemCount();
	QTreeWidgetItem* pItem;
	double v;

	for (j = 0; j < n; j++)
	{
		pItem = ui.tree->topLevelItem(j);
		v = QString2Double(pItem->text(5));
		pItem->setText(5, QString("%L1").arg(Round(v)));
	}	
}

void ZProtokol::updateAllSumm(const QList<int>& cols)
{
	int i, j, n = ui.tree->topLevelItemCount();
	QTreeWidgetItem* pItem;
	double v, s;
	bool fRound = ui.ckbRound->isChecked();
	QTreeWidgetItem* pSummItem = ui.tree->topLevelItem(n - 1);
	QFont fnt = pSummItem->font(0);

	foreach(i, cols)
	{
		v = 0;
		for (j = 0; j < n - 1; j++)
		{
			pItem = ui.tree->topLevelItem(j);
			if(i == 3 || i == 4)
				s = pItem->data(i, PAYMENT_ROLE).toDouble();
			else
				s = QString2Double(pItem->text(i));
			v += s;
		}
#ifndef MONEY_FORMAT
		pSummItem->setText(i, fRound ? QString::number(Round(v)) : QString::number(v, 'f', 2));
#else
		pSummItem->setText(i, fRound ? QString("%L1").arg(Round(v)) : QString("%L1").arg(v, 0, 'f', 2));
#endif
		pSummItem->setFont(i, fnt);
}
}

void ZProtokol::updateSumm()
{
	int i, j, n = ui.tree->topLevelItemCount();
	QTreeWidgetItem* pItem;
	double v;

	for (j = 0; j < n; j++)
	{
		pItem = ui.tree->topLevelItem(j);

		v = getSumma(pItem, 2);
#ifndef MONEY_FORMAT
		pItem->setText(2, QString::number(v, 'f', 2));
#else
		pItem->setText(2, QString("%L1").arg(v, 0, 'f', 2));
#endif

		for (i = 3; i < 5; i++)
		{
			v += pItem->data(i, PAYMENT_ROLE).toDouble();
		}

#ifndef MONEY_FORMAT
		pItem->setText(5, QString::number(v, 'f', 2));
#else
		pItem->setText(5, QString("%L1").arg(v, 0, 'f', 2));
#endif
		if (v < 0)
			pItem->setBackground(5, QColor(255, 170, 127));
	}

	if (ui.ckbRound->isChecked())
		roundSumm();

	QList<int> cols;
	cols << 2 << 3 << 4 << 5;
	updateAllSumm(cols);
}

int ZProtokol::getTextForPayment(int id, int col, QString &text, QVariantList &vList, double& summa)
{
	vList.clear();
	summa = 0;

	QString stringQuery = QString("SELECT payments2fio.id, dt, payments.name, val  FROM payments2fio INNER JOIN payments ON(payments.id = payment) WHERE fio = %1 AND ").arg(id);
	switch (col)
	{
	case 3://Бонусы
		stringQuery += "payments.mode = 0";
//		stringQuery += "payments.mode = 0 AND payment <> 0";
		break;
//	case 3://Аванс
//		stringQuery += "payment = 0";
//		break;
	case 4://Вычеты
		stringQuery += "payments.mode = 1";
		break;
	default:
		return 0;
	}

	stringQuery += QString(" AND dt_link >= '%1' AND dt_link <= '%2' ORDER BY dt")
		.arg(ui.dateStart->date().toString(DATE_FORMAT))
		.arg(ui.dateEnd->date().toString(DATE_FORMAT));

	QSqlQuery query;
	if (!query.exec(stringQuery))
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text());
		return 0;
	}

	QString txt;
	int pay_id;
	double v;
	QStringList sList;

	while (query.next())
	{
		pay_id = query.value(0).toInt();
		txt = QString("%1 %2 ").arg(query.value(1).toString()).arg(query.value(2).toString());
		
		v = query.value(3).toDouble();
#ifndef MONEY_FORMAT
		txt += QString::number(v, 'f', 2);
#else
		txt += QString("%L1").arg(v, 0, 'f', 2);
#endif
		summa += v;

		sList.push_back(txt);
		vList.push_back(pay_id);
	}

	if (col == 4)//Вычеты
		summa *= -1;

	text = sList.join("\n");
	return 1;
}

int ZProtokol::updateComment(int id, const QString & text)
{
	QString stringQuery = QString("UPDATE fio SET comment='%1' WHERE id=%2").arg(text).arg(id);
	QSqlQuery query;
	if (!query.exec(stringQuery))
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text());
		return 0;
	}
	return 1;
}

void ZProtokol::saveProtokol()
{
	QString fileName = ui.dateEnd->date().toString(DATE_FORMAT) + "-" + ui.dateStart->date().toString(DATE_FORMAT);

	fileName = QFileDialog::getSaveFileName(this, "Выбор файла для экспорта", fileName, "XLSX-файлы (*.xlsx)");
	if (fileName.isEmpty())
		return;

	int mode = QMessageBox::question(this, tr("Вариант формирования"), tr("Сохраняем полный или сокращенный вариант?"), tr("Полный"), tr("Сокращенный"), QString::null, 0, 1);
	if (mode != 0 && mode != 1)
		return;

	if(!fileName.endsWith(".xlsx", Qt::CaseInsensitive))
	    fileName += ".xlsx"; 

	QXlsx::Document xlsxW;

	Format fBold;
	fBold.setFontBold(true);

	xlsxW.write(1, 1, "Начало периода:", fBold);
	xlsxW.write(1, 2, ui.dateStart->date());
	xlsxW.write(2, 1, "Окончание периода:", fBold);
	xlsxW.write(2, 2, ui.dateEnd->date());

	xlsxW.write(3, 1, "Группа:", fBold);
	xlsxW.write(3, 2, ui.cboFilter->currentText());

	int i, j, n_child, k, n = ui.tree->topLevelItemCount();
	QTreeWidgetItem* pItem;

	fBold.setHorizontalAlignment(Format::AlignHCenter);

	pItem = ui.tree->headerItem();
	int colunms = pItem->columnCount();

	int curRow = 5;

	for (i = 0; i < colunms; i++)
	{
		xlsxW.write(curRow, i + 1, pItem->text(i), fBold);
		switch (i)
		{
		case 0:
		case 2:
		case 5:
		case 8:
		case 9:
			xlsxW.setColumnWidth(i + 1, 30);
			break;
		default:
			xlsxW.setColumnWidth(i + 1, 50);
			break;
		}
	}
	curRow++;

	QVariant v;
	QString s;
	Format fMultiLine;
	fMultiLine.setTextWrap(true);
	Format fMoney, fMoneyMinus;
	fMoney.setNumberFormat("#,##0.00\"р.\"");
	
	fMoneyMinus = fMoney;
	fMoneyMinus.setPatternBackgroundColor(QColor(255, 170, 127));
	
	QTreeWidgetItem* pItemChild;

	for (i = 0; i < n; i++)
	{
		if (i == n - 1)
		{
			fMultiLine.setFontBold(true);
			fMoneyMinus.setFontBold(true);
			fMoney.setFontBold(true);
		}

		pItem = ui.tree->topLevelItem(i);

		for (j = 0; j < colunms; j++)
		{
			v = pItem->data(j, Qt::DisplayRole);
			if (j == 2 || j == 5 || j == 9 || (i == n-1 && (j == 3 || j == 4)))
			{
				s = v.toString();
				v = QString2Double(s);
				xlsxW.write(curRow, j + 1, v, (i != n - 1 && v < 0) ? fMoneyMinus : fMoney);
			}
			else
				xlsxW.write(curRow, j + 1, v, fMultiLine);
		}
		curRow++;

		if (mode == 1) //Сокращенный
			continue;

		int curRowSave = curRow;

		n_child = pItem->childCount();
		for (k = 0; k < n_child; k++)
		{
			pItemChild = pItem->child(k);

			for (j = 0; j < colunms; j++)
			{
				v = pItemChild->data(j, Qt::DisplayRole);
				if (j == 2)
				{
					s = v.toString();
					v = QString2Double(s);
					xlsxW.write(curRow, j + 1, v, fMoney);
				}
				else
					xlsxW.write(curRow, j + 1, v, fMultiLine);
			}
			curRow++;
		}
		xlsxW.groupRows(curRowSave, curRow - 1);
	}
	xlsxW.currentWorksheet()->setFrozenRows(5);
	xlsxW.currentWorksheet()->setFrozenColumns(2);
	xlsxW.currentWorksheet()->setFilterRange(QXlsx::CellRange(5, 2, curRow - 1, 2));

	xlsxW.autosizeColumnWidth(1, colunms);
	xlsxW.setDocumentProperty("title", "Офигенный отчет");
	xlsxW.setDocumentProperty("subject", "А вам слабо!?");
	xlsxW.setDocumentProperty("company", "zaz@29.ru");
	xlsxW.setDocumentProperty("category", "Example spreadsheets");
	xlsxW.setDocumentProperty("keywords", "ZAZ");
	xlsxW.setDocumentProperty("creator", "ZAZ");
	xlsxW.setDocumentProperty("description", "Create by ZAZ");
	xlsxW.saveAs(fileName);

	QDesktopServices::openUrl(QUrl("file:///" + fileName, QUrl::TolerantMode));
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
ZTreeDataDelegate::ZTreeDataDelegate(ZProtokol* Editor, QObject* parent)
	: QItemDelegate(parent), pEditor(Editor)
{
	listWidget = NULL;
	w = NULL;
	textEdit = NULL;
}

QWidget* ZTreeDataDelegate::createEditor(QWidget* parent,
	const QStyleOptionViewItem& option,
	const QModelIndex& index) const
{
	column = index.column();

	fio_id = index.model()->data(index, FIO_ID_ROLE).toInt();
	
	textEdit = NULL;
	w = NULL;
	listWidget = NULL;

	if (fio_id > 0)
	{
		if (column > 2 && column < 5)
		{
			w = new QWidget(parent);
			w->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

			QPushButton* tb1 = new QPushButton(w);
			tb1->setText("+");
			tb1->setObjectName("tb1");
			tb1->setFixedWidth(20);
			connect(tb1, SIGNAL(clicked()), this, SLOT(add_clicked()));

			QPushButton* tb2 = new QPushButton(w);
			tb2->setText("...");
			tb2->setObjectName("tb2");
			tb2->setFixedWidth(20);
			connect(tb2, SIGNAL(clicked()), this, SLOT(edit_clicked()));

			QPushButton* tb3 = new QPushButton(w);
			tb3->setText("-");
			tb3->setObjectName("tb3");
			tb3->setFixedWidth(20);
			connect(tb3, SIGNAL(clicked()), this, SLOT(del_clicked()));

			listWidget = new QListWidget(w);

			QGridLayout* pLayout = new QGridLayout(w);
			pLayout->setSizeConstraint(QLayout::SetNoConstraint);
			pLayout->setMargin(0);
			pLayout->setSpacing(0);
			pLayout->addWidget(listWidget, 0, 0, 3, 1);
			pLayout->addWidget(tb1, 0, 1, Qt::AlignTop);
			pLayout->addWidget(tb2, 1, 1, Qt::AlignTop);
			pLayout->addWidget(tb3, 2, 1, Qt::AlignTop);
			return w;
		}
		if (column == 6)
		{
			textEdit = new QTextEdit(parent);
			textEdit->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
			return textEdit;
		}
	}
	return NULL;
}

void ZTreeDataDelegate::setEditorData(QWidget* editor,
	const QModelIndex& index) const
{
	if (listWidget)
	{
		listWidget->clear();
		QStringList items = index.model()->data(index, Qt::DisplayRole).toString().split("\n");
		QVariantList vList = index.model()->data(index, PAYMENT_ID_ROLE).toList();
		int i = 0;
		if (vList.size() != items.size())
			return;
		foreach(QVariant v, vList)
		{
			QListWidgetItem* pItem = new QListWidgetItem(items[i++]);
			pItem->setData(Qt::UserRole, v);
			listWidget->addItem(pItem);
		}
		return;
	}
	if (textEdit)
	{
		QString txt = index.model()->data(index, Qt::DisplayRole).toString();
		textEdit->setText(txt);
		return;
	}
	QItemDelegate::setEditorData(editor, index);
}

void ZTreeDataDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
	const QModelIndex& index) const
{
	if (listWidget)
	{
		QVariantList vList;
		QString txt;
		double v;

		int col = index.column();

		pEditor->getTextForPayment(fio_id, col, txt, vList, v);

		model->setData(index, vList, PAYMENT_ID_ROLE);
		model->setData(index, v, PAYMENT_ROLE);
		model->setData(index, txt, Qt::EditRole);

		pEditor->updateSumm();
		return;
	}
	if (textEdit)
	{
		QString txt = textEdit->toPlainText();

		if(pEditor->updateComment(fio_id, txt))
			model->setData(index, txt, Qt::EditRole);

		return;
	}
	QItemDelegate::setModelData(editor, model, index);
}

void ZTreeDataDelegate::updateEditorGeometry(QWidget* editor,
	const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	editor->setGeometry(option.rect);
}

void ZTreeDataDelegate::add_clicked()
{
	if (!listWidget || !w)
		return;

	if (openEditor(ADD_UNIC_CODE) == 0)
		return;

	commitData(w);
}

void ZTreeDataDelegate::edit_clicked()
{
	if (!listWidget || !w)
		return;
	QListWidgetItem* pItem = listWidget->currentItem();
	if (!pItem)
		return;
	int id = pItem->data(Qt::UserRole).toInt();

	if (openEditor(id) == 0)
		return;

	commitData(w);
}

void ZTreeDataDelegate::del_clicked()
{
	if (!listWidget || !w)
		return;

	QListWidgetItem* pItem = listWidget->currentItem();
	if (!pItem)
		return;
	int id = pItem->data(Qt::UserRole).toInt();

	QSqlQuery m_Query;
	if (!m_Query.exec(QString("DELETE FROM payments2fio WHERE id=%1").arg(id)))
	{
		ZMessager::Instance().Message(_Error, m_Query.lastError().text());
		return;
	}
	commitData(w);
}

int ZTreeDataDelegate::openEditor(int id)
{
	ZPayments2FioForm* pD = new ZPayments2FioForm(listWidget->parentWidget());
	pD->init("payments2fio", id);

	switch (column)
	{
	case 3://Бонусы
		pD->ui.cboMode->setCurrentIndex(0);
//		pD->ui.cboPayment->removeItem(0);
		break;
//	case 3://Аванс
//		pD->ui.cboPayment->setEnabled(false);
//		break;
	case 4://Вычеты
		pD->ui.cboMode->setCurrentIndex(1);
		break;
	default:
		return 0;
	}
	pD->ui.cboFIO->setEnabled(false);
	pD->ui.cboMode->setEnabled(false);
	pD->ui.dateLinkEdit->setMinimumDate(pEditor->ui.dateStart->date());
	pD->ui.dateLinkEdit->setMaximumDate(pEditor->ui.dateEnd->date());
	pD->ui.dateLinkEdit->setDate(pEditor->ui.dateEnd->date());

	if(id== ADD_UNIC_CODE)
		pD->ui.cboFIO->setCurrentIndex(pD->ui.cboFIO->findData(fio_id));

	if (pD->exec() != QDialog::Accepted)
		return 0;

	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
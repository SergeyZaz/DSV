#include <QSqlQuery>
#include <QSqlError>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSettings>


#include "zprotokol.h"
#include "zmessager.h"
#include "ztariffs.h"
#include "zpayments2fioform.h"
#include "zsettings.h"
#include "xlsxdocument.h"
using namespace QXlsx;

#define HIGHLIGHT_LABEL		"!!!"
#define HIGHLIGHT_COLOR		QColor(0, 128, 255)

#define FIO_ID_ROLE			Qt::UserRole
#define PAYMENT_ID_ROLE		Qt::UserRole+1
#define PAYMENT_ROLE		Qt::UserRole+2
#define COUNT_SMENS_ROLE	Qt::UserRole+3
#define BONUS_ROLE			Qt::UserRole+4
#define TARIFF_ROLE			Qt::UserRole+5
#define COUNT_ROLE			Qt::UserRole+6


#define DATE_COLUMN				0
#define FIO_COLUMN				1
#define PAYMENT_COLUMN			2
#define BONUS_COLUMN			3
#define MINUS_COLUMN			4
#define BALANCE_COLUMN			5
#define COMMENT_COLUMN			6
#define NOTES_COLUMN			7
#define NUM_SMENS_COLUMN		8
#define MIDDLE_SMENS_COLUMN		9
#define INDEX_COLUMN			10

int Round(double dVal)
{
	return ((int)dVal / 100 + ((int)dVal % 100 >= 50 ? 1 : 0)) * 100;
//	if ((dVal - (int)dVal) >= 0.5)
//		return (int)ceil(dVal);
//	else
//		return (int)floor(dVal);
}

QString oldMemo;

ZProtokol::ZProtokol(QWidget* parent, Qt::WindowFlags flags)//: QDialog(parent, flags)
{
	ui.setupUi(this);
	//setAttribute(Qt::WA_DeleteOnClose);

	QSettings settings("Zaz", "DSV");
	ui.dateStart->setDate(settings.value("dateBegin", QDate::currentDate().addMonths(-1)).toDate());
	ui.dateEnd->setDate(settings.value("dateEnd", QDate::currentDate()).toDate());
	
	ui.lblReadOnly->setVisible(false);

	connect(ui.cmdBuild, SIGNAL(clicked()), this, SLOT(buildProtokol()));
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(saveProtokol()));
	connect(ui.ckbExpandAll, SIGNAL(clicked(bool)), this, SLOT(expandAll(bool)));
	connect(ui.cmdFind, SIGNAL(clicked()), this, SLOT(findNextSlot()));
	connect(ui.txtFind, SIGNAL(textChanged(const QString&)), this, SLOT(findFirstSlot(const QString&)));

	ui.tree->setColumnWidth(INDEX_COLUMN, 20);
	ui.tree->setColumnWidth(DATE_COLUMN, 200);
	ui.tree->setColumnWidth(FIO_COLUMN, 200);
	ui.tree->setColumnWidth(PAYMENT_COLUMN, 250);
	ui.tree->setColumnWidth(BONUS_COLUMN, 250);
	ui.tree->setColumnWidth(MINUS_COLUMN, 300);
	ui.tree->setColumnWidth(BALANCE_COLUMN, 200);

	ui.tree->setItemDelegate(new ZTreeDataDelegate(this, ui.tree));


	QList<QAction*> contextMnuActions;
	contextMnuActions.append(ui.actChangeOrg);
	ui.tree->setContextMenuPolicy(Qt::ActionsContextMenu);
	ui.tree->addActions(contextMnuActions);
	connect(ui.actChangeOrg, SIGNAL(triggered()), this, SLOT(changeOrgSlot()));

	loadItemsToComboBox(ui.cboFilter, "groups");
	loadItemsToComboBox(ui.cboFilterOrg, "organisation");
	loadItemsToComboBox(ui.cboFilterFIO, "fio");
	
	buildProtokol();

	ui.tree->header()->moveSection(INDEX_COLUMN, 1);

	curFindId = -1;

	QSqlQuery query;
	if (!query.exec("SELECT value FROM config WHERE key = 'memo'"))
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), "Ошибка");
	}

	if (query.next())
	{
		oldMemo = query.value(0).toString();
		ui.txtMemo->setPlainText(oldMemo);
	}

	ui.txtMemo->setReadOnly(ZSettings::Instance().m_UserType == 1);

}


ZProtokol::~ZProtokol()
{
	if (ZSettings::Instance().m_UserType == 1)
		return;

	if (oldMemo == ui.txtMemo->toPlainText())
		return;

	if (QMessageBox::question(this, tr("Внимание"), tr("Вы действительно хотите изменить комментарий?"), tr("Да"), tr("Нет"), QString::null, 0, 1) != 0)
		return;
	
	QSqlQuery query;
	if (!query.exec("DELETE FROM config WHERE key = 'memo'"))
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
		return;
	}

	if (!query.exec(QString("INSERT INTO config(key, value) VALUES('memo','%1')").arg(ui.txtMemo->toPlainText())))
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
		return;
	}
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
	QApplication::setOverrideCursor(Qt::WaitCursor);

	QProgressDialog progress("Получение данных...", "Отмена", 0, 0, QApplication::activeWindow());
	progress.setWindowModality(Qt::WindowModal);
	progress.show();
	QApplication::processEvents();

	loadTariffs();

	QString dateStartStr = ui.dateStart->date().toString(DATE_FORMAT);
	QString dateEndStr = ui.dateEnd->date().toString(DATE_FORMAT);
	
	QSettings settings("Zaz", "DSV");
	settings.setValue("dateBegin", ui.dateStart->date());
	settings.setValue("dateEnd", ui.dateEnd->date());

	ZSettings::Instance().f_ReadOnly = ZSettings::Instance().m_CloseDate.isValid() && (ui.dateStart->date() < ZSettings::Instance().m_CloseDate);
	ui.lblReadOnly->setVisible(ZSettings::Instance().f_ReadOnly || ZSettings::Instance().m_UserType == 1);

	QString stringQuery = QString("SELECT dt, import_data.fio, fio.name, organisation.id, organisation.name, smena, smena.name, tariff, num, groups.id, fio.comment  FROM import_data \
INNER JOIN fio ON(import_data.fio = fio.id) \
LEFT JOIN organisation2fio ON(import_data.fio = value) \
LEFT JOIN organisation ON(organisation2fio.key = organisation.id) \
INNER JOIN smena ON(smena.id = smena) \
LEFT JOIN groups2fio ON(import_data.fio = groups2fio.value) \
LEFT JOIN groups ON(groups2fio.key = groups.id) \
WHERE dt >= '%1' AND dt <= '%2' ORDER BY fio.name,dt,smena")
.arg(dateStartStr)
.arg(dateEndStr);

	QSqlQuery query;
	if (!query.exec(stringQuery))
	{
		progress.close();
		QApplication::restoreOverrideCursor();
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
		return;
	}

	ui.tree->clear();
	ui.ckbExpandAll->setCheckState(Qt::Unchecked);

	QTreeWidgetItem *pItem, *pItemGroup;
	QMap<int, QTreeWidgetItem*> mapFIO;
	int id, num;
	double v, bonus, tV;
	QString txt, comm;
	QDate date;
	int i, j, n;
	QVariantList vList;
	QStringList sList;
	bool isSmena;
	int allRowNum = 0;

	int groupId = ui.cboFilter->currentData().toInt();
	int organisationId = ui.cboFilterOrg->currentData().toInt();
	int fioId = ui.cboFilterFIO->currentData().toInt();

	while (query.next())
	{
		if (groupId != 0 && groupId != query.value(9).toInt())
			continue;
		if (organisationId != 0 && organisationId != query.value(3).toInt())
			continue;

		id = query.value(1).toInt();

		if (fioId != 0 && fioId != id)
			continue;

		pItemGroup = mapFIO.value(id);
		if (!pItemGroup)
		{
			pItemGroup = new QTreeWidgetItem(ui.tree);
			pItemGroup->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);

			pItemGroup->setText(DATE_COLUMN, query.value(4).toString());
			pItemGroup->setText(FIO_COLUMN, query.value(2).toString());
			pItemGroup->setData(FIO_COLUMN, FIO_ID_ROLE, id);

			txt = query.value(10).toString();
			pItemGroup->setText(COMMENT_COLUMN, txt);
			if(txt.contains(HIGHLIGHT_LABEL))
				pItemGroup->setData(FIO_COLUMN, Qt::BackgroundColorRole, HIGHLIGHT_COLOR);

			pItemGroup->setText(INDEX_COLUMN, QString::number(++allRowNum));

			ui.tree->addTopLevelItem(pItemGroup);
			mapFIO.insert(id, pItemGroup);

			comm.clear();

			for (i = BONUS_COLUMN; i < BALANCE_COLUMN; i++)
			{
				pItemGroup->setData(i, FIO_ID_ROLE, id);

				getTextForPayment(id, i, txt, vList, v, comm);

				pItemGroup->setText(i, txt);
				pItemGroup->setData(i, PAYMENT_ID_ROLE, vList);

				pItemGroup->setData(i, PAYMENT_ROLE, v);
			}
			pItemGroup->setData(6, FIO_ID_ROLE, id);

			pItemGroup->setSizeHint(DATE_COLUMN, QSize(100, 50));

			QString stringQuery2 = QString("SELECT note FROM notes2fio WHERE fio = %1 AND end_dt >= '%2' AND begin_dt <= '%3'")
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
				pItemGroup->setText(NOTES_COLUMN, comm + stringQuery2);
			}
		}

		pItem = new QTreeWidgetItem(pItemGroup);

		date = query.value(0).toDate();
		pItem->setText(DATE_COLUMN, date.toString(DATE_FORMAT));
		pItem->setText(FIO_COLUMN, query.value(6).toString());

		id = query.value(7).toInt();
		num = query.value(8).toDouble();

		v = getTariffValue(date, id, num, txt, bonus, tV, isSmena);
		pItem->setData(DATE_COLUMN, BONUS_ROLE, bonus);
		pItem->setData(DATE_COLUMN, TARIFF_ROLE, tV);
		pItem->setData(DATE_COLUMN, COUNT_ROLE, isSmena ? 1 : num);

		// если в названии тарифа есть слово "смена" то считаю количество - числом смен
		pItem->setData(DATE_COLUMN, COUNT_SMENS_ROLE, txt.contains("смена") ? num : 1);

		pItem->setText(BONUS_COLUMN, txt);
		
#ifndef MONEY_FORMAT
		pItem->setText(PAYMENT_COLUMN, QString::number(v, 'f', 2));
#else
		pItem->setText(PAYMENT_COLUMN, QString("%L1").arg(v, 0, 'f', 2));
#endif
	}

	//пересчитываю доплаты, которые начислились в один день (заменяю на средние значения)
	n = ui.tree->topLevelItemCount();
	for (j = 0; j < n; j++)
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
				if (pIt1->text(DATE_COLUMN) == pIt2->text(DATE_COLUMN) && pIt1->text(FIO_COLUMN) == pIt2->text(FIO_COLUMN))
					l_Itms << pIt2;
			}

			int s = l_Itms.size();
			if (s == 0)
				continue;
			
			i += s;
			
			int nn = l_Itms[0]->data(DATE_COLUMN, COUNT_SMENS_ROLE).toInt();
			l_Itms[0]->setData(DATE_COLUMN, COUNT_SMENS_ROLE, QString::number(nn - s));

			l_Itms << pIt1;
			s++;

			double s_d = 0;
			foreach(pIt1, l_Itms)
				s_d += pIt1->data(DATE_COLUMN, BONUS_ROLE).toDouble();
			s_d /= s * s; //среднее на строку
			foreach(pIt1, l_Itms)
			{
				v = QString2Double(pIt1->text(PAYMENT_COLUMN)) - pIt1->data(DATE_COLUMN, BONUS_ROLE).toDouble() + s_d;
#ifndef MONEY_FORMAT
				pIt1->setText(PAYMENT_COLUMN, QString::number(v, 'f', 2));
#else
				pIt1->setText(PAYMENT_COLUMN, QString("%L1").arg(v, 0, 'f', 2));
#endif
				pIt1->setData(DATE_COLUMN, BONUS_ROLE, s_d);
			}
		}
	}

	//добавляю формулу расчета начислений
	for (j = 0; j < n; j++)
	{
		pItem = ui.tree->topLevelItem(j);
		if (!pItem)
			continue;
		int childs = pItem->childCount();
		for (i = 0; i < childs; i++)
		{
			QTreeWidgetItem* pIt1 = pItem->child(i);

			bonus = pIt1->data(DATE_COLUMN, BONUS_ROLE).toDouble();
			num = pIt1->data(DATE_COLUMN, COUNT_ROLE).toInt();
			tV = pIt1->data(DATE_COLUMN, TARIFF_ROLE).toDouble();
			pIt1->setText(MINUS_COLUMN, QString("(%1 * %2) + %3").arg(tV, 0, 'f', 2).arg(num).arg(bonus, 0, 'f', 2).replace(".", ","));
		}
	}

	for (int zz = 0; zz < 2; zz++)
	{
		stringQuery = (zz == 0) ? 

			//добавляю ФИО для которых нет смен но есть выплаты!
			QString("SELECT payments2fio.fio, fio.name, organisation.id, organisation.name, groups.id, fio.comment FROM payments2fio \
INNER JOIN fio ON(payments2fio.fio = fio.id) \
LEFT JOIN organisation2fio ON(payments2fio.fio = value) \
LEFT JOIN organisation ON(organisation2fio.key = organisation.id) \
LEFT JOIN groups2fio ON(payments2fio.fio = groups2fio.value) \
LEFT JOIN groups ON(groups2fio.key = groups.id) \
WHERE dt_link >= '%1' AND dt_link <= '%2' ORDER BY fio.name")
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
WHERE end_dt >= '%1' AND begin_dt <= '%2' AND char_length(note) > 0 ORDER BY fio.name")
.arg(dateStartStr)
.arg(dateEndStr);

		if (!query.exec(stringQuery))
		{
			progress.close();
			QApplication::restoreOverrideCursor();
			ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
			continue;
		}
		while (query.next())
		{
			if (groupId != 0 && groupId != query.value(4).toInt())
				continue;
			if (organisationId != 0 && organisationId != query.value(2).toInt())
				continue;

			id = query.value(0).toInt();

			if (fioId != 0 && fioId != id)
				continue;

			pItemGroup = mapFIO.value(id);
			if (!pItemGroup)
			{
				pItemGroup = new QTreeWidgetItem(ui.tree);
				pItemGroup->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);

				pItemGroup->setText(DATE_COLUMN, query.value(3).toString());
				pItemGroup->setText(FIO_COLUMN, query.value(1).toString());
				pItemGroup->setData(FIO_COLUMN, FIO_ID_ROLE, id);

				txt = query.value(5).toString();
				pItemGroup->setText(COMMENT_COLUMN, txt);
				if (txt.contains(HIGHLIGHT_LABEL))
					pItemGroup->setData(FIO_COLUMN, Qt::BackgroundColorRole, HIGHLIGHT_COLOR);

				pItemGroup->setText(INDEX_COLUMN, QString::number(++allRowNum));

				ui.tree->addTopLevelItem(pItemGroup);
				mapFIO.insert(id, pItemGroup);

				comm.clear();

				for (i = BONUS_COLUMN; i < BALANCE_COLUMN; i++)
				{
					pItemGroup->setData(i, FIO_ID_ROLE, id);

					getTextForPayment(id, i, txt, vList, v, comm);

					pItemGroup->setText(i, txt);
					pItemGroup->setData(i, PAYMENT_ID_ROLE, vList);

					pItemGroup->setData(i, PAYMENT_ROLE, v);
				}
				pItemGroup->setData(6, FIO_ID_ROLE, id);

				pItemGroup->setSizeHint(DATE_COLUMN, QSize(100, 50));

				QString stringQuery2 = QString("SELECT note FROM notes2fio WHERE fio = %1 AND end_dt >= '%2' AND begin_dt <= '%3'")
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
					pItemGroup->setText(NOTES_COLUMN, comm + stringQuery2);
				}
			}
		}
	}
	
	QFont fnt;
	pItem = new QTreeWidgetItem(ui.tree);
	pItem->setText(0, "Итого:");
	ui.tree->addTopLevelItem(pItem);
	fnt = pItem->font(DATE_COLUMN);
	fnt.setPointSizeF(14);
	fnt.setBold(true);
	pItem->setFont(DATE_COLUMN, fnt);

	updateSumm();

	n = ui.tree->topLevelItemCount();
	for (j = 0; j < n - 1; j++)
	{
		pItem = ui.tree->topLevelItem(j);

		for (i = 0; i < pItem->columnCount(); i++)
		{
			fnt = pItem->font(i);
			if (i > PAYMENT_COLUMN && i < BALANCE_COLUMN || i == COMMENT_COLUMN)
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
			smens += pItemGroup->data(DATE_COLUMN, COUNT_SMENS_ROLE).toInt();
		}
		pItem->setText(NUM_SMENS_COLUMN, QString::number(smens));

		//среднее за смену
		if (smens > 0)
		{
			v = QString2Double(pItem->text(PAYMENT_COLUMN)) / smens;
#ifndef MONEY_FORMAT
			pItem->setText(MIDDLE_SMENS_COLUMN, QString::number(v, 'f', 2));
#else
			pItem->setText(MIDDLE_SMENS_COLUMN, QString("%L1").arg(v, 0, 'f', 2));
#endif
		}
	}

	for(i = 0; i < ui.tree->columnCount(); i++)
		ui.tree->resizeColumnToContents(i);
	
	progress.close();
	QApplication::restoreOverrideCursor();
}

void ZProtokol::roundSumm()
{
	int j, n = ui.tree->topLevelItemCount();
	QTreeWidgetItem* pItem;
	double v;

	for (j = 0; j < n; j++)
	{
		pItem = ui.tree->topLevelItem(j);
		v = QString2Double(pItem->text(BALANCE_COLUMN));
		pItem->setText(BALANCE_COLUMN, QString("%L1").arg(Round(v)));
	}	
}

void ZProtokol::updateAllSumm(const QList<int>& cols)
{
	int i, j, n = ui.tree->topLevelItemCount();
	QTreeWidgetItem* pItem;
	double v, s;
	bool fRound = ui.ckbRound->isChecked();
	QTreeWidgetItem* pSummItem = ui.tree->topLevelItem(n - 1);
	QFont fnt = pSummItem->font(DATE_COLUMN);

	foreach(i, cols)
	{
		v = 0;
		for (j = 0; j < n - 1; j++)
		{
			pItem = ui.tree->topLevelItem(j);
			if(i == BONUS_COLUMN || i == MINUS_COLUMN)
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

		v = getSumma(pItem, PAYMENT_COLUMN);
#ifndef MONEY_FORMAT
		pItem->setText(PAYMENT_COLUMN, QString::number(v, 'f', 2));
#else
		pItem->setText(PAYMENT_COLUMN, QString("%L1").arg(v, 0, 'f', 2));
#endif

		for (i = BONUS_COLUMN; i < BALANCE_COLUMN; i++)
		{
			v += pItem->data(i, PAYMENT_ROLE).toDouble();
		}

#ifndef MONEY_FORMAT
		pItem->setText(BALANCE_COLUMN, QString::number(v, 'f', 2));
#else
		pItem->setText(BALANCE_COLUMN, QString("%L1").arg(v, 0, 'f', 2));
#endif
		if (v < 0)
			pItem->setBackground(BALANCE_COLUMN, QColor(255, 170, 127));
	}

	if (ui.ckbRound->isChecked())
		roundSumm();

	QList<int> cols;
	cols << PAYMENT_COLUMN << BONUS_COLUMN << MINUS_COLUMN << BALANCE_COLUMN;
	updateAllSumm(cols);
}

int ZProtokol::getTextForPayment(int id, int col, QString &text, QVariantList &vList, double& summa, QString& comm)
{
	vList.clear();
	summa = 0;

	QString stringQuery = QString("SELECT payments2fio.id, dt, payments.name, val, payments2fio.comment  FROM payments2fio INNER JOIN payments ON(payments.id = payment) WHERE fio = %1 AND ").arg(id);
	switch (col)
	{
	case BONUS_COLUMN://Бонусы
		stringQuery += "payments.mode = 0";
		break;
	case MINUS_COLUMN://Вычеты
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

		txt = query.value(4).toString();
		if (!txt.isEmpty())
		{
			comm += txt;
			comm += " \n";
		}
	}

	if (col == MINUS_COLUMN)//Вычеты
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
	xlsxW.write(4, 1, "Организация:", fBold);
	xlsxW.write(4, 2, ui.cboFilterOrg->currentText());

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
		case DATE_COLUMN:
		case PAYMENT_COLUMN:
		case BALANCE_COLUMN:
		case NUM_SMENS_COLUMN:
		case MIDDLE_SMENS_COLUMN:
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
			if (j == PAYMENT_COLUMN || j == BALANCE_COLUMN || j == MIDDLE_SMENS_COLUMN || (i == n-1 && (j == BONUS_COLUMN || j == MINUS_COLUMN)))
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
				if (j == PAYMENT_COLUMN)
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
	xlsxW.currentWorksheet()->setFrozenColumns(PAYMENT_COLUMN);
	xlsxW.currentWorksheet()->setFilterRange(QXlsx::CellRange(5, PAYMENT_COLUMN, curRow - 1, PAYMENT_COLUMN));

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

void ZProtokol::findFirstSlot(const QString& text)
{
	curFindId = -1;
	findText(text);
}

void ZProtokol::findNextSlot()
{
	QString text = ui.txtFind->text();
	findText(text);
}

int ZProtokol::findText(const QString& text)
{
	ui.tree->clearSelection();

	bool fExist = false;
	QTreeWidgetItemIterator iT(ui.tree);
	while (*iT)
	{
		if ((*iT)->text(FIO_COLUMN).contains(text, Qt::CaseInsensitive) || (*iT)->text(DATE_COLUMN).contains(text, Qt::CaseInsensitive))
		{
			if (curFindId != -1)
			{
				if (curFindId == (*iT)->data(FIO_COLUMN, FIO_ID_ROLE).toInt())
					curFindId = -1;
				iT++;
				continue;
			}
			curFindId = (*iT)->data(FIO_COLUMN, FIO_ID_ROLE).toInt();
			ui.tree->scrollToItem(*iT);
			ui.tree->setItemSelected(*iT, true);
			fExist = true;
			break;
		}
		iT++;
	}

	if (!fExist && curFindId != -1)
	{
		curFindId = -1;
		findText(text);
	}
	return 1;
}

void ZProtokol::changeOrgSlot()
{
	QList<QTreeWidgetItem*> sItems = ui.tree->selectedItems();
	if (sItems.size() == 0)
		return;
	int curId = sItems.at(0)->data(FIO_COLUMN, FIO_ID_ROLE).toInt();

	QSqlQuery query;
	QStringList l_organisation;
	if (query.exec("SELECT name FROM organisation ORDER BY name"))
		while (query.next())
			l_organisation.push_back(query.value(0).toString());

	bool ok;
	QString str_query, str = getZItem(this, QString("Изменение организации"),
		QString("Организации:"), l_organisation, 0, true, &ok, Qt::MSWindowsFixedSizeDialogHint);
	if (!ok || str.isEmpty())
		return;

	str_query = QString("DELETE FROM organisation2fio WHERE value=%1;").arg(curId);
	if (!query.exec(str_query))
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text());
		return;
	}

	if (str != "не задано")
	{
		str_query = QString("INSERT INTO organisation2fio (key,value) VALUES((SELECT id FROM organisation WHERE name='%1'), %2);")
			.arg(str)
			.arg(curId);
		if (!query.exec(str_query))
		{
			ZMessager::Instance().Message(_CriticalError, query.lastError().text());
			return;
		}
	}
	buildProtokol();
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
	if (ZSettings::Instance().f_ReadOnly || ZSettings::Instance().m_UserType == 1)
		return NULL;

	column = index.column();

	fio_id = index.model()->data(index, FIO_ID_ROLE).toInt();
	
	textEdit = NULL;
	w = NULL;
	listWidget = NULL;

	if (fio_id > 0)
	{
		if (column > PAYMENT_COLUMN && column < BALANCE_COLUMN)
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
		if (column == COMMENT_COLUMN)
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
		QString txt, comm;
		double v;

		int col = index.column();

		pEditor->getTextForPayment(fio_id, col, txt, vList, v, comm);

		model->setData(index, vList, PAYMENT_ID_ROLE);
		model->setData(index, v, PAYMENT_ROLE);
		model->setData(index, txt, Qt::EditRole);

		pEditor->updateSumm();
		return;
	}
	if (textEdit)
	{
		QString txt = textEdit->toPlainText();

		if (pEditor->updateComment(fio_id, txt))
		{
			model->setData(index, txt, Qt::EditRole);
			if (txt.contains(HIGHLIGHT_LABEL))
				model->setData(model->index(index.row(), FIO_COLUMN, index.parent()), HIGHLIGHT_COLOR, Qt::BackgroundColorRole);
		}

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
	
	pD->ui.cboFIO->setEnabled(false);
	pD->ui.cboMode->setEnabled(false);
//	pD->ui.dateLinkEdit->setMinimumDate(pEditor->ui.dateStart->date());
//	pD->ui.dateLinkEdit->setMaximumDate(pEditor->ui.dateEnd->date());
	pD->ui.dateLinkEdit->setDate(pEditor->ui.dateEnd->date());
	pD->ui.dateEdit->setDate(pEditor->ui.dateStart->date());
	
	pD->init("payments2fio", id);

	if (id == ADD_UNIC_CODE)
	{
		pD->ui.cboFIO->setCurrentIndex(pD->ui.cboFIO->findData(fio_id));

		switch (column)
		{
		case BONUS_COLUMN://Бонусы
			pD->ui.cboMode->setCurrentIndex(0);
			break;
		case MINUS_COLUMN://Вычеты
			pD->ui.cboMode->setCurrentIndex(1);
			break;
		default:
			return 0;
		}
	}

	if (pD->exec() != QDialog::Accepted)
		return 0;

	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
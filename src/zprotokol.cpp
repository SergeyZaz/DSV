#include <QSqlQuery>
#include <QSqlError>
#include <QFileDialog>
#include <QDesktopServices>
#include "zprotokol.h"
#include "zmessager.h"
#include "ztariffs.h"
#include "zpayments2fioform.h"

#include "xlsxdocument.h"
using namespace QXlsx;

#define FIO_ID_ROLE			Qt::UserRole
#define PAYMENT_ID_ROLE		Qt::UserRole+1
#define PAYMENT_ROLE		Qt::UserRole+2

ZProtokol::ZProtokol(QWidget* parent, Qt::WindowFlags flags)//: QDialog(parent, flags)
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
	ui.tree->setColumnWidth(4, 300);

	ui.tree->setItemDelegate(new ZTreeDataDelegate(this, ui.tree));

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

double QString2Double(QString txt)
{
	bool ok;	
	txt.replace(QChar::Nbsp, "");
	double v = txt.toDouble(&ok);
	if (!ok)
	{
		txt.replace(",", ".");
		v = txt.toDouble(&ok);
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

	QString stringQuery = QString("SELECT dt, import_data.fio, fio.name, organisation.id, organisation.name, smena, smena.name, tariff, num, groups.id, fio.comment  FROM import_data \
INNER JOIN fio ON(import_data.fio = fio.id) \
INNER JOIN organisation2fio ON(import_data.fio = value) \
INNER JOIN organisation ON(organisation2fio.key = organisation.id) \
INNER JOIN smena ON(smena.id = smena) \
LEFT JOIN groups2fio ON(import_data.fio = groups2fio.value) \
LEFT JOIN groups ON(groups2fio.key = groups.id) \
WHERE dt >= '%1' AND dt <= '%2' ORDER BY fio.name,dt")
.arg(ui.dateStart->date().toString(DATE_FORMAT))
.arg(ui.dateEnd->date().toString(DATE_FORMAT));

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
	int i, j, n;
	QVariantList vList;
	QStringList sList;

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

			for (i = 2; i < 5; i++)
			{
				pItemGroup->setData(i, FIO_ID_ROLE, id);

				getTextForPayment(id, i, txt, vList, v);

				pItemGroup->setText(i, txt);
				pItemGroup->setData(i, PAYMENT_ID_ROLE, vList);

				pItemGroup->setData(i, PAYMENT_ROLE, v);
			}
			pItemGroup->setData(6, FIO_ID_ROLE, id);

			pItemGroup->setSizeHint(0, QSize(100, 50));
		}

		pItem = new QTreeWidgetItem(pItemGroup);

		date = query.value(0).toDate();
		pItem->setText(0, date.toString(DATE_FORMAT));
		pItem->setText(1, query.value(6).toString());

		id = query.value(7).toInt();
		num = query.value(8).toDouble();

		pItem->setText(2, QString::number(num));

		v = getTariffValue(date, id, num, txt, bonus);
		pItem->setText(3, txt);
#ifndef MONEY_FORMAT
		pItem->setText(5, QString::number(v, 'f', 2));
#else
		pItem->setText(5, QString("%L1").arg(v, 0, 'f', 2));
#endif
	}

	QFont fnt;
	n = ui.tree->topLevelItemCount();
	for (j = 0; j < n; j++)
	{
		pItem = ui.tree->topLevelItem(j);

		for (i = 0; i < pItem->columnCount(); i++)
		{
			fnt = pItem->font(i);
			if (i > 1 && i < 5 || i == 6)
				fnt.setPointSizeF(8);
			else
				fnt.setBold(true);
			pItem->setFont(i, fnt);
		}
	}

	updateSumm();
}

void ZProtokol::updateSumm()
{
	int i, j, n = ui.tree->topLevelItemCount();
	QTreeWidgetItem* pItem;
	double v;

	for (j = 0; j < n; j++)
	{
		pItem = ui.tree->topLevelItem(j);

		v = getSumma(pItem, 5);

		for (i = 2; i < 5; i++)
		{
			v += pItem->data(i, PAYMENT_ROLE).toDouble();
		}

#ifndef MONEY_FORMAT
		pItem->setText(5, QString::number(v, 'f', 2));
#else
		pItem->setText(5, QString("%L1").arg(v, 0, 'f', 2));
#endif
	}
}

int ZProtokol::getTextForPayment(int id, int col, QString &text, QVariantList &vList, double& summa)
{
	vList.clear();
	summa = 0;

	QString stringQuery = QString("SELECT payments2fio.id, dt, payments.name, val  FROM payments2fio INNER JOIN payments ON(payments.id = payment) WHERE fio = %1 AND ").arg(id);
	switch (col)
	{
	case 2://Доплаты
		stringQuery += "payments.mode = 0 AND payment <> 0";
		break;
	case 3://Аванс
		stringQuery += "payment = 0";
		break;
	case 4://Вычеты
		stringQuery += "payments.mode = 1";
		break;
	default:
		return 0;
	}

	stringQuery += QString(" AND dt >= '%1' AND dt <= '%2' ORDER BY dt")
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

	int i, j, n = ui.tree->topLevelItemCount();
	QTreeWidgetItem* pItem;

	fBold.setHorizontalAlignment(Format::AlignHCenter);

	xlsxW.write(5, 1, "Организация", fBold);
	xlsxW.setColumnWidth(1, 30);
	xlsxW.write(5, 2, "ФИО", fBold);
	xlsxW.setColumnWidth(2, 50);
	xlsxW.write(5, 3, "Доплаты", fBold);
	xlsxW.setColumnWidth(3, 50);
	xlsxW.write(5, 4, "Аванс", fBold);
	xlsxW.setColumnWidth(4, 50);
	xlsxW.write(5, 5, "Вычеты", fBold);
	xlsxW.setColumnWidth(5, 50);
	xlsxW.write(5, 6, "Сумма", fBold);
	xlsxW.setColumnWidth(6, 20);
	xlsxW.write(5, 7, "примечания", fBold);
	xlsxW.setColumnWidth(7, 50);

	QVariant v;
	QString s;
	Format fMultiLine;
	fMultiLine.setTextWrap(true);
	Format fMoney;
	fMoney.setNumberFormat("#,##0.00\"р.\"");

	for (i = 0; i < n; i++)
	{
		pItem = ui.tree->topLevelItem(i);

		for (j = 0; j < 7; j++)
		{
			v = pItem->data(j, Qt::DisplayRole);
			if (j == 5)
			{
				s = v.toString();
				v = QString2Double(s);
				xlsxW.write(i + 6, j + 1, v, fMoney);
			}
			else
				xlsxW.write(i + 6, j + 1, v, fMultiLine);
		}
	}

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
		if (column > 1 && column < 5)
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
	case 2://Доплаты
		pD->ui.cboMode->setCurrentIndex(0);
		pD->ui.cboPayment->removeItem(0);
		break;
	case 3://Аванс
		pD->ui.cboPayment->setEnabled(false);
		break;
	case 4://Вычеты
		pD->ui.cboMode->setCurrentIndex(1);
		break;
	default:
		return 0;
	}
	pD->ui.cboFIO->setEnabled(false);
	pD->ui.cboMode->setEnabled(false);
	pD->ui.dateEdit->setMinimumDate(pEditor->ui.dateStart->date());
	pD->ui.dateEdit->setMaximumDate(pEditor->ui.dateEnd->date());

	if(id== ADD_UNIC_CODE)
		pD->ui.cboFIO->setCurrentIndex(pD->ui.cboFIO->findData(fio_id));

	if (pD->exec() != QDialog::Accepted)
		return 0;

	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
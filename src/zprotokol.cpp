#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include "zprotokol.h"
#include "zfilteroperations.h"

ZProtokol::ZProtokol(QSqlDatabase &m_db, QWidget *parent)
	: db(m_db), QDialog(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	loadItemsToListBox(ui.listUrPersons, "ur_persons");
	loadItemsToListBox(ui.listProjects, "projects");
	loadItemsToListBox(ui.listAccounts, "accounts");

	ui.dateStart->setDate(QDate(QDate::currentDate().year(), 1, 1));
	ui.dateEnd->setDate(QDate::currentDate());
	
	setModal(true);

	connect(ui.cmdBuild, SIGNAL(clicked()), this, SLOT(buildProtokol()));
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(saveProtokol()));	
	connect(ui.tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(openFilterProtokol(QTreeWidgetItem *, int)));	
}

ZProtokol::~ZProtokol()
{

}

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

void ZProtokol::saveProtokol()
{
	QString fileName = QFileDialog::getSaveFileName(this);
	if (fileName.isEmpty()) 
		return;

#ifdef SAVETOXML
	if(!fileName.endsWith(".xlsx"))
		fileName += ".xlsx";

	saveToXLSFile(fileName);
	return;
#endif //SAVETOXML

	if(!fileName.endsWith(".csv"))
		fileName += ".csv";

	saveToFile(fileName);

}

void ZProtokol::buildProtokol()
{
	int m_SectionsMode = ui.cboSections->currentIndex(); // 0-доходы и расходы, 1-доходы, 2-расходы
	int m_Inderval = ui.cboInterval->currentIndex();
	QDate m_dStart = ui.dateStart->date();
	QDate m_dStop = ui.dateEnd->date();

	int i,numInterval;

	switch(m_Inderval)
	{
	case 0://по месяцам
		numInterval = (m_dStop.year() - m_dStart.year()) * 12 + m_dStop.month() - m_dStart.month() + 1;
		break;
	case 1://по кварталам
		numInterval = (m_dStop.year() - m_dStart.year()) * 4 + (m_dStop.month() / 4)  - (m_dStart.month() / 4) + 1;
		break;
	case 2://по годам
		numInterval = m_dStop.year() - m_dStart.year() + 1;
		break;
	default:
		return;
	}

	ui.tree->setColumnCount(numInterval + 1);
	QTreeWidgetItem *pHeader = ui.tree->headerItem();

	QList<QDate> l_Intervals;
	QStringList l_Mounths, l_Quarts;
	l_Mounths << "" << tr("январь ") << tr("февраль ") << tr("март ") << tr("апрель ") << tr("май ") << tr("июнь ") << tr("июль ") << tr("август ") << tr("сент¤брь ") << tr("окт¤брь ") << tr("ноябрь ") << tr("декабрь ");
	l_Quarts << "I." << "II."<< "III."<< "IV.";
	QDate tmpDate;
	QString text;

	switch(m_Inderval)
	{
	case 0://по мес¤цам
		tmpDate = QDate(m_dStart.year(), m_dStart.month(), 1);
		l_Intervals << tmpDate;
		break;
	case 1://по кварталам
		tmpDate = QDate(m_dStart.year(), (m_dStart.month() / 4) + 1, 1);
		l_Intervals << tmpDate;
		break;
	case 2://по годам
		tmpDate = QDate(m_dStart.year(), 1, 1);
		l_Intervals << tmpDate;
		break;
	}

	for(i=0; i<numInterval; i++)
	{
		switch(m_Inderval)
		{
		case 0://по мес¤цам
			text = l_Mounths.at(tmpDate.month()) + QString::number(tmpDate.year());
			tmpDate = tmpDate.addMonths(1);
			break;
		case 1://по кварталам
			text = l_Quarts.at(tmpDate.month() / 4) + QString::number(tmpDate.year());
			tmpDate = tmpDate.addMonths(3);
			break;
		case 2://по годам
			text = QString::number(tmpDate.year());
			tmpDate = tmpDate.addMonths(12);
			break;
		}
		l_Intervals << tmpDate;
		pHeader->setText(i+1, text);
	}
	
	ui.tree->setColumnWidth(0, 200);

	puts("----------------");
	foreach(QDate d, l_Intervals)
		puts(d.toString("yyyy-MM-dd").toLocal8Bit().data());
	puts("++++++++++++++++");

	ui.tree->clear();
	
	QTreeWidgetItem *pItem, *pItemRoot, *pItemGroup;
	QSqlQuery query1, query2;
	int rc, id, j;
	double summa;
	QListWidgetItem *pListItem;
	QString sUrPersons, sProjects, sAccounts, sOperations;
	QFont fnt;	

	//////////////////// фильтры ////////////////////
	sAccounts = "";
	for(i=0; i<ui.listAccounts->count(); i++)
	{
		pListItem = ui.listAccounts->item(i);
		if(pListItem->checkState() == Qt::Checked)
			sAccounts += QString::number(pListItem->data(Qt::UserRole).toInt()) + ",";
	}
	sAccounts.chop(1);

	sUrPersons = "";
	for(i=0; i<ui.listUrPersons->count(); i++)
	{
		pListItem = ui.listUrPersons->item(i);
		if(pListItem->checkState() == Qt::Checked)
			sUrPersons += QString::number(pListItem->data(Qt::UserRole).toInt()) + ",";
	}
	sUrPersons.chop(1);

	sProjects = "";
	for(i=0; i<ui.listProjects->count(); i++)
	{
		pListItem = ui.listProjects->item(i);
		if(pListItem->checkState() == Qt::Checked)
			sProjects += QString::number(pListItem->data(Qt::UserRole).toInt()) + ",";
	}
	sProjects.chop(1);
	//////////////////////////////////////////////////

	for(int ii=0; ii<3; ii++)
	{
		pItemGroup = new QTreeWidgetItem(ui.tree);
		switch(ii)
		{
		case 1:
			pItemGroup->setText(0, tr("инвестиционный поток"));
			break;
		case 2:
			pItemGroup->setText(0, tr("финансовый поток"));
			break;
		default:
			pItemGroup->setText(0, tr("операционный поток"));
			break;
		}
		fnt = pItemGroup->font(0);
		fnt.setBold(true);
		pItemGroup->setFont(0, fnt);
		//pItemGroup->setData(0, Qt::UserRole+1, ii);
		ui.tree->addTopLevelItem(pItemGroup);
		
		for(i=0; i<2; i++)
		{
			switch(m_SectionsMode)
			{
				case 1:// доходы
					if(i==1)
					{
						i++;
						continue;
					}
					break;
				case 2:// расходы
					i = 1;
					break;
				default:// доходы и расходы
					break;
			}
			pItemRoot = new QTreeWidgetItem(pItemGroup);
			pItemRoot->setText(0, i == 0 ? tr("ѕоступлени¤") : tr("¬ыплаты"));
			pItemRoot->setData(0, Qt::UserRole, i == 0 ? 1 : -1);
			ui.tree->addTopLevelItem(pItemRoot);

			rc = query1.exec(QString("SELECT id,name FROM sections WHERE type=%1 AND parent=%2 ORDER BY name ").arg(i).arg(ii));
			if (rc)
			{
				while (query1.next()) 
				{
					pItem = new QTreeWidgetItem(pItemRoot);
					pItem->setText(0, query1.value(1).toString().simplified());
					pItem->setData(0, Qt::UserRole, i == 0 ? 1 : -1);
					id = query1.value(0).toInt();
					//pItem->setData(0, Qt::UserRole+1, id);

					for(j=1; j<l_Intervals.size(); j++)
					{
						summa = 0;
						text = QString("SELECT val,id FROM operations WHERE type=%1 AND ur_person IN (%2) AND project IN (%3) AND account IN (%4) AND date >= '%5' AND date < '%6' AND section=%7")
							.arg(i)
							.arg(sUrPersons)
							.arg(sProjects)
							.arg(sAccounts)
							.arg(l_Intervals.at(j-1).toString("yyyy-MM-dd"))
							.arg(l_Intervals.at(j).toString("yyyy-MM-dd"))
							.arg(id);
						if (query2.exec(text))
						{
							sOperations = "";
							while (query2.next()) 
							{
								summa += query2.value(0).toDouble();
								sOperations += query2.value(1).toString() + ",";
							}
							sOperations.chop(1);
						}

#ifndef MONEY_FORMAT
						pItem->setText(j, QString::number(summa, 'f', 2));
#else
						pItem->setText(j, QString("%L1").arg(summa, 0, 'f', 2));
#endif
						pItem->setData(j, Qt::UserRole, sOperations);
						pItem->setTextAlignment(j, Qt::AlignRight);
					}
				}
			}
		}
	}

	//////////////////////// перемещения ////////////////////////
	pItemGroup = new QTreeWidgetItem(ui.tree);
	pItemGroup->setText(0, tr("перемещения"));
	pItemGroup->setFont(0, fnt);
	ui.tree->addTopLevelItem(pItemGroup);

	for(i=0; i<2; i++)
	{
		switch(m_SectionsMode)
		{
		case 1:// доходы
			if(i==1)
			{
				i++;
				continue;
			}
			break;
		case 2:// расходы
			i = 1;
			break;
		default:// доходы и расходы
			break;
		}
		pItemRoot = new QTreeWidgetItem(pItemGroup);
		pItemRoot->setText(0, i == 0 ? tr("зачисления") : tr("списания"));
		pItemRoot->setData(0, Qt::UserRole, i == 0 ? 1 : -1);
		ui.tree->addTopLevelItem(pItemRoot);

		for(j=1; j<l_Intervals.size(); j++)
		{
			summa = 0;
			text = QString("SELECT val FROM operations WHERE type=%1 AND ur_person IN (%2) AND project IN (%3) AND account IN (%4) AND date >= '%5' AND date < '%6' AND section IN (SELECT id FROM sections WHERE type=%7)")
				.arg(2)
				.arg(sUrPersons)
				.arg(sProjects)
				.arg(sAccounts)
				.arg(l_Intervals.at(j-1).toString("yyyy-MM-dd"))
				.arg(l_Intervals.at(j).toString("yyyy-MM-dd"))
				.arg(i);
			if (query2.exec(text))
			{
				while (query2.next()) 
				{
					summa += query2.value(0).toDouble();
				}
			}

#ifndef MONEY_FORMAT
			pItemRoot->setText(j, QString::number(summa, 'f', 2));
#else
			pItemRoot->setText(j, QString("%L1").arg(summa, 0, 'f', 2));
#endif
			pItemRoot->setTextAlignment(j, Qt::AlignRight);
		}
	}

	//////////////////////// начальный остаток ////////////////////////
	double summaAll = 0;
	for(i=0; i<2; i++)
	{
		text = QString("SELECT sum(val) FROM operations WHERE type=%1 AND ur_person IN (%2) AND project IN (%3) AND account IN (%4) AND date < '%5'")
			.arg(i) //“ип: 0-ѕоступление/1-¬ыплата/2-ѕеремещение
			.arg(sUrPersons)
			.arg(sProjects)
			.arg(sAccounts)
			.arg(l_Intervals.at(0).toString("yyyy-MM-dd"));
		if (query2.exec(text))
		{
			while (query2.next()) 
			{
				if(i==0)
					summaAll += query2.value(0).toDouble();
				else
					summaAll -= query2.value(0).toDouble();
			}
		}
	}
	///////////////////////////////////////////////////////////////////


	QTreeWidgetItem *pItemItog = new QTreeWidgetItem(ui.tree);
	ui.tree->addTopLevelItem(pItemItog);
	pItemItog->setText(0, tr("общий денежный поток"));
	pItemItog->setFont(0, fnt);
	
	QTreeWidgetItem *pItemDelta = new QTreeWidgetItem(ui.tree);
	ui.tree->addTopLevelItem(pItemDelta);
	pItemDelta->setText(0, tr("остатки на конец периода"));
	pItemDelta->setFont(0, fnt);
	
	int n = ui.tree->topLevelItemCount();
	for(i=1; i<l_Intervals.size(); i++)
	{
		summa = 0;

		for(j=0; j<n; j++)
		{
			summa += getSumma(ui.tree->topLevelItem(j), i);
		}

#ifndef MONEY_FORMAT
		pItemItog->setText(i, QString::number(summa, 'f', 2));
#else
		pItemItog->setText(i, QString("%L1").arg(summa, 0, 'f', 2));
#endif
		pItemItog->setFont(i, fnt);
		pItemItog->setTextAlignment(i, Qt::AlignRight);

		summaAll += summa;
#ifndef MONEY_FORMAT
		pItemDelta->setText(i, QString::number(summaAll, 'f', 2));
#else
		pItemDelta->setText(i, QString("%L1").arg(summaAll, 0, 'f', 2));
#endif
		pItemDelta->setFont(i, fnt);
		pItemDelta->setTextAlignment(i, Qt::AlignRight);

		ui.tree->resizeColumnToContents(i);
	}
	
	ui.tree->expandAll();
}
	
double ZProtokol::getSumma(QTreeWidgetItem *pItemRoot, int col)
{
	double s = 0;

	if(!pItemRoot)
		return s;

	QString str = pItemRoot->text(0);

	int n = pItemRoot->childCount();
	for(int i=0; i<n; i++)
	{
		s += getSumma(pItemRoot->child(i), col);
	}

	if(n > 0)
	{
		double v = s;
		if(!pItemRoot->font(0).bold())
			v = fabs(s);
#ifndef MONEY_FORMAT
		pItemRoot->setText(col, QString::number(v, 'f', 2));
#else
		pItemRoot->setText(col, QString("%L1").arg(v, 0, 'f', 2));
#endif
		pItemRoot->setTextAlignment(col, Qt::AlignRight);
	}
	else
	{
		double v = pItemRoot->text(col).replace(QChar::Nbsp, "").toDouble();
		int k = pItemRoot->data(0, Qt::UserRole).toInt();
		if(k!=0)
			s += v * k;
	}
	return s;
}
	
int ZProtokol::saveToFile(const QString &fileName)
{
	int cols = ui.tree->headerItem()->columnCount();

	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QMessageBox::critical(NULL, QString("ошибка"), QString("ошибка создания файла!"));
		return 0;
	}

	QTextStream out(&file);	 
	int i, j, k, l, n = ui.tree->topLevelItemCount();
	QTreeWidgetItem *pItemRoot, *pItem, *pItemChild;

	for(j=0; j<cols; j++)
	{
		out << ui.tree->headerItem()->text(j) << ";";
		if(j == 0)
			out << ";;";
	}
	out << "\n";

	for(i=0;i<n;i++)
	{
		pItemRoot = ui.tree->topLevelItem(i);
		for(j=0; j<cols; j++)
		{
			out << pItemRoot->text(j).replace(".", ",").replace(QChar::Nbsp, "") << ";";
			if(j == 0)
				out << ";;";
		}
		out << "\n";

		for(k=0; k<pItemRoot->childCount(); k++)
		{
			pItem = pItemRoot->child(k);
			for(j=0; j<cols; j++)
			{
				if(j == 0)
					out << ";";
				out << pItem->text(j).replace(".", ",").replace(QChar::Nbsp, "") << ";";
				if(j == 0)
					out << ";";
			}
			out << "\n";

			for(j=0; j<pItem->childCount(); j++)
			{
				pItemChild = pItem->child(j);
				for(l=0; l<cols; l++)
				{
					if(l == 0)
						out << ";;";
					out << pItemChild->text(l).replace(".", ",").replace(QChar::Nbsp, "") << ";";
				}
				out << "\n";
			}
		}
	}

	return 1;
}

#ifdef SAVETOXML
#include <QAxObject>
void setVal2Cell(QAxObject* StatSheet, int row, int col, QString &s)
{
	if(!StatSheet)
		return;
	// получение указател¤ на ¤чейку [row][col] ((!)нумераци¤ с единицы)
	QAxObject *cell = StatSheet->querySubObject("Cells(Int,Int)", row, col);
	// вставка значени¤ переменной data (любой тип, приводимый к QVariant) в полученную ¤чейку
	if(!cell)
		return;
	s = s.replace(QChar::Nbsp, "");
	bool ok;
	double v = s.toDouble(&ok);
	if(ok)
		cell->dynamicCall("SetValue(const QVariant&)", v);
	else						
		cell->dynamicCall("SetValue(const QVariant&)", s);
    delete cell;
}
#endif //SAVETOXML

int ZProtokol::saveToXLSFile(const QString &fileName)
{
#ifdef SAVETOXML
	int cols = ui.tree->headerItem()->columnCount();

	// получаем указатель на Excel
    QAxObject *mExcel = new QAxObject("Excel.Application",this);
	//делаем его видимым
	//mExcel->dynamicCall( "SetVisible(bool)", TRUE ); 
    // получаем указатель на книги
    QAxObject *workbooks = mExcel->querySubObject("Workbooks");
    // добавл¤ем книгу
	workbooks->dynamicCall("Add");
    // получаем указатель на текущую книгу
	QAxObject *workbook = mExcel->querySubObject("ActiveWorkBook");
	if(!workbook)
		return 0;
    // получаем указатель на листы
    QAxObject *mSheets = workbook->querySubObject( "Sheets" );
    // указываем, какой лист выбрать
    QAxObject *StatSheet = mSheets->querySubObject( "Item(const QVariant&)", QVariant("Ћист1") );
	if(!StatSheet)
		return 0;

	int i, j, k, l, n = ui.tree->topLevelItemCount();
	QTreeWidgetItem *pItemRoot, *pItem, *pItemChild;

	int row = 1, col = 1;
	for(j=0; j<cols; j++)
	{
		setVal2Cell(StatSheet, row, col, ui.tree->headerItem()->text(j));
		if(j == 0)
			col+=2;
		col++;
	}
		
	row++;

	for(i=0;i<n;i++)
	{
		pItemRoot = ui.tree->topLevelItem(i);
		col = 1;
		for(j=0; j<cols; j++)
		{
			setVal2Cell(StatSheet, row, col, pItemRoot->text(j));
			if(j == 0)
				col+=2;
			col++;
		}
		row++;

		for(k=0; k<pItemRoot->childCount(); k++)
		{
			pItem = pItemRoot->child(k);
			col = 1;
			for(j=0; j<cols; j++)
			{
				if(j == 0)
					col++;
				setVal2Cell(StatSheet, row, col, pItem->text(j));
				if(j == 0)
					col++;
				col++;
			}
			row++;

			for(j=0; j<pItem->childCount(); j++)
			{
				pItemChild = pItem->child(j);
				col = 1;
				for(l=0; l<cols; l++)
				{
					if(l == 0)
						col+=2;
					setVal2Cell(StatSheet, row, col, pItemChild->text(l));
					col++;
				}
				row++;
			}
		}
	}

	QString tmp_s = fileName;
	//правим слеши, насколько необходимо не помню
	tmp_s.replace("/","\\");
	//говорим excel что вс¤кие служебные сообщени¤ выводить не надо
	mExcel->setProperty("DisplayAlerts", 0);
	//сохран¤ем наш файл под сгенерированным именем в каталог с программой
	workbook->dynamicCall("SaveAs (const QString&)", tmp_s);

	mExcel->setProperty("DisplayAlerts", 1);

	//зачем-то надо, канонично сразу не закомменитировал, теперь не помн
	workbook->dynamicCall("Close(Boolean)", false);

    delete StatSheet;
    delete mSheets;
    delete workbook;
    delete workbooks;
	delete mExcel;

	QMessageBox::information(this, QString("¬ыполнено"), QString("ѕротокол сохранен!"));
#endif //SAVETOXML
	return 1;
}

void ZProtokol::openFilterProtokol(QTreeWidgetItem *item, int column)
{
	QString sOperations = item->data(column, Qt::UserRole).toString();
	if(sOperations.isEmpty() || column==0)
		return;
	ZFilterOperations *pD = new ZFilterOperations(this);
	pD->initDB(db, sOperations);
	pD->setWindowTitle(QString("%1 (—умма: %2)").arg(item->text(0)).arg(item->text(column)));
	pD->resize(1000, 200);
	pD->exec();
}

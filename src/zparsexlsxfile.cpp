#include <QApplication>
#include <QString>
#include <QSqlQuery>
#include <QSqlError>
#include <QProgressDialog>
#include <QCompleter>
#include <QDialogButtonBox>
#include "zparsexlsxfile.h"
#include "zsettings.h"
#include "zmessager.h"
#include "zview.h"

#include "xlsxdocument.h"
//#include "xlsxchartsheet.h"
//#include "xlsxcellrange.h"
//#include "xlsxchart.h"
//#include "xlsxrichstring.h"
#include "xlsxworkbook.h"
using namespace QXlsx;

#define IMPORT_TAG_DT "dt"
#define IMPORT_TAG_SMENA "smena"
#define IMPORT_TAG_WORK "work"
#define IMPORT_TAG_FIO "fio"
#define IMPORT_TAG_NUM "num"
#define IMPORT_TAG_TYRE "tyre"
#define IMPORT_TAG_RATE "rate"

bool ZParseXLSXFile::loadFile(const QString& fileName)
{
	QApplication::setOverrideCursor(Qt::WaitCursor);

	QProgressDialog progress("Обработка файла...", "Отмена", 0, 0, QApplication::activeWindow());
	progress.setWindowModality(Qt::WindowModal);
	progress.show();

	Document xlsxR(fileName);

	//int sheetIndexNumber = 0;
	//foreach(QString currentSheetName, xlsxR.sheetNames())
	QStringList sheetNames = xlsxR.sheetNames();
	if (sheetNames.size() == 0)
	{
		progress.close();
		QApplication::restoreOverrideCursor();
		ZMessager::Instance().Message(_Error, "В файле не найдено ни одного листа", "Ошибка");
		return false;
	}
	QString currentSheetName = sheetNames[0];

	AbstractSheet* currentSheet = xlsxR.sheet(currentSheetName);
	if (!currentSheet)
	{
		progress.close();
		QApplication::restoreOverrideCursor();
		ZMessager::Instance().Message(_Error, "Невозможно получить данные", "Ошибка");
		return false;
	}

	currentSheet->workbook()->setActiveSheet(0);
	Worksheet* wsheet = (Worksheet*)currentSheet->workbook()->activeSheet();
	if (!wsheet)
	{
		progress.close();
		QApplication::restoreOverrideCursor();
		ZMessager::Instance().Message(_Error, "Невозможно получить данные с листа", "Ошибка");
		return false;
	}

	QString strSheetName = wsheet->sheetName();

	QVector<CellLocation> clList = wsheet->getFullCells(&maxRow, &maxCol);
	for (int rc = 0; rc < maxRow; rc++)
	{
		QVector<QVariant> tempValue;

		for (int cc = 0; cc < maxCol; cc++)
		{
			tempValue.push_back(QVariant());
		}
		m_Data.push_back(tempValue);
	}

	for (int ic = 0; ic < clList.size(); ++ic)
	{
		QApplication::processEvents();
		if (progress.wasCanceled())
			break;

		CellLocation cl = clList.at(ic);

		int row = cl.row - 1;
		int col = cl.col - 1;

		QSharedPointer<Cell> ptrCell = cl.cell;

		if (ptrCell->isDateTime())
			m_Data[row][col] = ptrCell.data()->dateTime();
		else
			m_Data[row][col] = ptrCell.data()->value();
	}

	if (m_Data.size() == 0)
	{
		progress.close();
		QApplication::restoreOverrideCursor();
		ZMessager::Instance().Message(_Error, "Содержимое файла не соответствует данным для импорта", "Ошибка");
		return false;
	}

	progress.close();
	return true;
}
	
bool ZParseXLSXFile::loadImportData(const QString & fileName)
{
	if (!loadFile(fileName))
		return false;

	QSqlQuery query;
	QString dt_str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
	QString str_query = QString("INSERT INTO public.import(file, dt) VALUES('%1', '%2')").arg(fileName).arg(dt_str);
	if (!query.exec(str_query))
		ZMessager::Instance().Message(_CriticalError, query.lastError().text());
	uint key = 0;
	str_query = QString("SELECT id FROM public.import WHERE dt='%1'").arg(dt_str);
	if (!query.exec(str_query) || !query.next())
		ZMessager::Instance().Message(_CriticalError, query.lastError().text());
	else
		key = query.value(0).toInt();

	int rc = insertData(key);

	str_query = QString("UPDATE public.import SET status=%1 WHERE id='%2'").arg(rc).arg(key);
	if (!query.exec(str_query))
		ZMessager::Instance().Message(_CriticalError, query.lastError().text());

	if (rc == 1)
		ZMessager::Instance().Message(_Warning, "Импортирование успешно выполнено", "Внимание");

	QApplication::restoreOverrideCursor();
	return (rc == 1);
}

int ZParseXLSXFile::insertData(uint key)
{
	QSqlQuery query;
	QString str_query, str_tmpl;
	int i, j;
	bool fNoData = true;

	QMap<QString, int> columnMap;
	auto iT = ZSettings::Instance().importTags.constBegin();
	while (iT != ZSettings::Instance().importTags.constEnd())
	{
		fNoData = true;
		i = 0;
		foreach(QVariant v, m_Data[0])
		{
			if (v.toString() == iT.key())
			{
				fNoData = false;
				columnMap.insert(iT.value(), i);
				break;
			}
			i++;
		}

		if (fNoData)
		{
			ZMessager::Instance().Message(_Error, "Структура файла не соответствует данным для импорта", "Ошибка");
			return 0;
		}

		++iT;
	}
	
	if (!query.exec("SELECT id,txt,mode,type,pr FROM tariff ORDER BY pr"))
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text());
		return 0;
	}

	QProgressDialog progress("Обработка данных...", "Отмена", 0, m_Data.size(), QApplication::activeWindow());
	progress.setWindowModality(Qt::WindowModal);
	progress.show();

	struct TariffInfo
	{
		int id;
		//0 - станок ..., 1 - станок начинается с ..., 2 - станок содержит ..., 3 - станок заканчивается ...,
		//4 - модель шины ..., 5 - модель шины начинается с ..., 6 - модель шины содержит ..., 7 - модель шины заканчивается ...,
		//8 - Rate ..., 9 - Rate начинается с ..., 10 - Rate содержит ..., 11 - Rate заканчивается ...		
		int mode;
		int type; // 0 - за смену, 1 - за штуку
		QString txt;
		int prioritet; // приоритет
	};
	QList<TariffInfo> Tariffs;

	while (query.next())
	{
		TariffInfo info;
		info.id = query.value(0).toInt();
		info.mode = query.value(2).toInt();
		info.txt = query.value(1).toString();
		info.type = query.value(3).toInt();
		info.prioritet = query.value(4).toInt();
		Tariffs << info;
	}
		
	int fio_id, tariff_id, tariff_type = 0;

	for (i = 1; i < m_Data.size(); i++)
	{
		QApplication::processEvents();
		if (progress.wasCanceled())
			break;
		progress.setValue(i);

		const QVector<QVariant>& row = m_Data[i];

		//разбираю тариф
		//0 - станок ..., 1 - станок начинается с ..., 2 - станок содержит ..., 3 - станок заканчивается ...,
		//4 - модель шины ..., 5 - модель шины начинается с ..., 6 - модель шины содержит ..., 7 - модель шины заканчивается ...,
		//8 - Rate ..., 9 - Rate начинается с ..., 10 - Rate содержит ..., 11 - Rate заканчивается ...		
		tariff_id = 0;
		foreach(TariffInfo tar, Tariffs)
		{
			switch (tar.mode)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				str_tmpl = row[columnMap[IMPORT_TAG_WORK]].toString();
				break;
			case 4:
			case 5:
			case 6:
			case 7:
				str_tmpl = row[columnMap[IMPORT_TAG_TYRE]].toString();
				break;
			case 8:
			case 9:
			case 10:
			case 11:
				str_tmpl = row[columnMap[IMPORT_TAG_RATE]].toString();
				break;
			default:
				continue;
			}

			fNoData = true;
			switch (tar.mode)
			{
			case 0:
			case 4:
			case 8:
				fNoData = !(str_tmpl.compare(tar.txt, Qt::CaseInsensitive) == 0);
				break;
			case 1:
			case 5:
			case 9:
				fNoData = !(str_tmpl.startsWith(tar.txt, Qt::CaseInsensitive));
				break;
			case 2:
			case 6:
			case 10:
				fNoData = !(str_tmpl.contains(tar.txt, Qt::CaseInsensitive));
				break;
			case 3:
			case 7:
			case 11:
				fNoData = !(str_tmpl.endsWith(tar.txt, Qt::CaseInsensitive));
				break;
			default:
				break;
			}
			if (!fNoData)
			{
				tariff_id = tar.id;
				tariff_type = tar.type;
				break;
			}
		}

		//работаем с fio
		fio_id = 0;
		str_tmpl = row[columnMap[IMPORT_TAG_FIO]].toString();
		if (str_tmpl.isEmpty())// могут быть пустые строки
			continue;

		if (tariff_id == 0) //значит неопознанный тариф
		{
			str_tmpl.clear();
			foreach(QVariant v, row)
				str_tmpl += v.toString() + " ";
			ZMessager::Instance().Message(_Error, QString("В строке %1, неопознанный вид тарифа: \"%2\"").arg(i + 1).arg(str_tmpl));
			continue;
		}

		str_query = QString("SELECT id FROM fio WHERE name='%1'").arg(str_tmpl);
		if (!query.exec(str_query) || !query.next())
		{
			QString str_query = QString("INSERT INTO fio(name) VALUES('%1')").arg(str_tmpl);
			if (!query.exec(str_query))
				ZMessager::Instance().Message(_CriticalError, QString("В строке %1: %2").arg(i + 1).arg(query.lastError().text()));
			else
			{
				str_query = QString("SELECT id FROM fio WHERE name='%1'").arg(str_tmpl);
				if (!query.exec(str_query) || !query.next())
					ZMessager::Instance().Message(_CriticalError, QString("В строке %1: %2").arg(i + 1).arg(query.lastError().text()));
				else
					fio_id = query.value(0).toInt();
			}
		}
		else
			fio_id = query.value(0).toInt();

		str_query = QString("INSERT INTO import_data (dt,smena,tariff,fio,num,import_id,row_num) VALUES('%1', (SELECT id FROM smena WHERE name='%2'), %3, %4, %5, %6, %7);")
			.arg(row[columnMap[IMPORT_TAG_DT]].toString())
			.arg(row[columnMap[IMPORT_TAG_SMENA]].toString())
			.arg(tariff_id)
			.arg(fio_id)
			.arg(tariff_type == 0 ? 0 : row[columnMap[IMPORT_TAG_NUM]].toInt())
			.arg(key)
			.arg(i+1);
		if (!query.exec(str_query))
		{
			//str_query = query.lastError().text();
			ZMessager::Instance().Message(_CriticalError, QString("В строке %1: %2").arg(i + 1).arg(query.lastError().text()));
		}
	}

	return 1;
}

QString getZItem(QWidget* parent, const QString& title, const QString& label,
	const QStringList& items, int current, bool editable, bool* ok,
	Qt::WindowFlags flags)
{
	QString text(items.value(current));

	QDialog *q = new QDialog(NULL, flags);
	q->setWindowTitle(title);
	QLabel *pLabel = new QLabel(label, q);
	pLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, q);
	QObject::connect(buttonBox, SIGNAL(accepted()), q, SLOT(accept()));
	QObject::connect(buttonBox, SIGNAL(rejected()), q, SLOT(reject()));

	QComboBox *comboBox = new QComboBox(q);
	comboBox->addItems(items);
	comboBox->setEditable(editable);
		
	QCompleter* completer = new QCompleter();
	completer->setModel(comboBox->model());
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	if (editable)
		comboBox->setCompleter(completer);

	QVBoxLayout *mainLayout = new QVBoxLayout(q);
	mainLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
	mainLayout->addWidget(pLabel);
	mainLayout->addWidget(comboBox);
	mainLayout->addWidget(buttonBox);

	QFont font = q->font();
	font.setPointSize(14);
	q->setFont(font);

	int ret = q->exec();
	if (ok)
		*ok = ret;
	if (ret) 
	{
		text = comboBox->currentText();
	}

	delete q;

	return text;
}

bool ZParseXLSXFile::loadPayments(const QString& fileName)
{
	if (!loadFile(fileName))
		return false;

	int i, fio, payment, rc = 0;
	QProgressDialog progress("Обработка данных...", "Отмена", 0, m_Data.size(), QApplication::activeWindow());
	progress.setWindowModality(Qt::WindowModal);
	progress.show();

	QString str_fio, str_payment, str_query;
	QSqlQuery query;

	QStringList l_fio;
	if (query.exec("SELECT name FROM fio ORDER BY name"))
		while (query.next())
			l_fio.push_back(query.value(0).toString());

	QStringList l_organisation;
	if (query.exec("SELECT name FROM organisation ORDER BY name"))
		while (query.next())
			l_organisation.push_back(query.value(0).toString());

	double sum = 0;
	QString str2;
	bool ok;

	for (i = 0; i < m_Data.size(); i++)
	{
		QApplication::processEvents();
		if (progress.wasCanceled())
			break;
		progress.setValue(i);

		const QVector<QVariant>& row = m_Data[i];
		if (row.size() < 5)
		{
			ZMessager::Instance().Message(_CriticalError, QString("В строке %1: недостаточно заполненных столбцов!").arg(i + 1));
			continue;
		}
		str_fio = row[1].toString().simplified();
		str_payment = row[2].toString();
		double v = QString2Double(row[3].toString());
		if (v == 0 || str_fio.isEmpty() || str_payment.isEmpty() || row[0].isNull() || row[4].isNull() || row[3].isNull())// могут быть пустые строки
		{
			ZMessager::Instance().Message(_CriticalError, QString("В строке %1: недостаточно данных!").arg(i + 1));
			continue;
		}

		str_query = QString("SELECT id FROM fio WHERE name='%1'").arg(str_fio);
		if (!query.exec(str_query) || !query.next())
		{
			str2 = str_fio;
			QStringList items_fio = str2.simplified().split(" ");
			if (items_fio.size() > 2)
				str2 = items_fio[0] + " " + items_fio[1];

			str_query = QString("SELECT id FROM fio WHERE name='%1'").arg(str2);
			if (!query.exec(str_query) || !query.next())
			{
				str2 = getZItem(NULL, QString("Внимание!"),
					QString("В справочнике 'Люди' не найдена запись: '%1'.\nНеобходимо выбрать из существующих:").arg(str_fio), l_fio, 0, true, &ok, Qt::MSWindowsFixedSizeDialogHint);
				if (!ok || str2.isEmpty())
				{
					ZMessager::Instance().Message(_CriticalError, QString("В справочнике 'Люди' не найдена запсись: '%1'").arg(str_fio));
					continue;
				}
				
				str_query = QString("SELECT id FROM fio WHERE name='%1'").arg(str2);
				if (!query.exec(str_query) || !query.next())
				{
					ZMessager::Instance().Message(_CriticalError, QString("В справочнике 'Люди' не найдена запсись: '%1'").arg(str_fio));
					continue;
				}
			}
		}
		fio = query.value(0).toInt();

		//проверка на привязку к организации
		str_query = QString("SELECT key FROM organisation2fio WHERE value=%1").arg(fio);
		if (!query.exec(str_query) || !query.next())
		{
			str2 = getZItem(NULL, QString("Внимание!"),
				QString("'%1' не призяван к организации.\nНеобходимо выбрать из существующих:").arg(str_fio), l_organisation, 0, true, &ok, Qt::MSWindowsFixedSizeDialogHint);
			if (ok && !str2.isEmpty())
			{
				str_query = QString("INSERT INTO organisation2fio (key,value) VALUES((SELECT id FROM organisation WHERE name='%1'), %2);")
					.arg(str2)
					.arg(fio);
				if (!query.exec(str_query))
					ZMessager::Instance().Message(_CriticalError, query.lastError().text());
			}
		}

		str_query = QString("SELECT id FROM payments WHERE name='%1'").arg(str_payment);
		if (!query.exec(str_query) || !query.next())
		{
			ZMessager::Instance().Message(_CriticalError, QString("В справочнике 'Выплаты' не найдена запсись: '%1'").arg(str_payment));
			continue;
		}
		payment = query.value(0).toInt();

		str_query = QString("INSERT INTO payments2fio (dt,fio,payment,val,dt_link) VALUES('%1', %2, %3, %4, '%5');")
			.arg(row[0].toString())
			.arg(fio)
			.arg(payment)
			.arg(v, 0, 'f', 2)
			.arg(row[4].toString());
		if (!query.exec(str_query))
		{
			ZMessager::Instance().Message(_CriticalError, QString("В строке %1: %2").arg(i+1).arg(query.lastError().text()));
		}
		else
		{
			sum += v;
			rc++;
		}
	}

	if (rc > 0)
		ZMessager::Instance().Message(_Warning, QString("Импортирование успешно выполнено, добавлено %1 записей, на сумму %2").arg(rc).arg(sum), "Внимание");
	QApplication::restoreOverrideCursor();
	return (rc > 0);
}

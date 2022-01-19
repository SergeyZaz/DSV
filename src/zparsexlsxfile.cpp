#include <QApplication>
#include <QString>
#include <QSqlQuery>
#include <QSqlError>
#include <QProgressDialog>
#include <QInputDialog>
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
	
	if (!query.exec("SELECT id,txt,mode FROM tariff"))
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
		int mode;
		QString txt;
	};
	QList<TariffInfo> Tariffs;

	while (query.next())
	{
		TariffInfo info;
		info.id = query.value(0).toInt();
		info.mode = query.value(2).toInt();
		info.txt = query.value(1).toString();
		Tariffs << info;
	}
		
	int fio_id, tariff_id;

	for (i = 1; i < m_Data.size(); i++)
	{
		QApplication::processEvents();
		if (progress.wasCanceled())
			break;
		progress.setValue(i);

		const QVector<QVariant>& row = m_Data[i];

		//разбираю тариф
		//mode: 0-столбец "станок" (вид работы), 1-столбец "модель шины" содержит txt, 2-столбец "модель шины" начинается с txt, 3-столбец "модель шины" заканчивается txt, 4-столбец "Rate" значение txt
		tariff_id = 0;
		// 1. проверяю столбец "Rate"
		foreach(TariffInfo tar, Tariffs)
		{
			if (tar.mode != 4)
				continue;
			if (row[columnMap[IMPORT_TAG_RATE]].toString().compare(tar.txt, Qt::CaseInsensitive)==0)// == tar.txt)
			{
				tariff_id = tar.id;
				break;
			}
		}
		// 2. проверяю столбец "станок / вид работы"
		if (tariff_id == 0)
		{
			foreach(TariffInfo tar, Tariffs)
			{
				if (tar.mode != 0)
					continue;
				if (row[columnMap[IMPORT_TAG_WORK]].toString().compare(tar.txt, Qt::CaseInsensitive) == 0)// == tar.txt)
				{
					tariff_id = tar.id;
					break;
				}
			}
		}
		str_tmpl = row[columnMap[IMPORT_TAG_TYRE]].toString();
		// 3. проверяю столбец "модель шины" заканчивается txt
		if (tariff_id == 0)
		{
			foreach(TariffInfo tar, Tariffs)
			{
				if (tar.mode != 3)
					continue;
				if (str_tmpl.endsWith(tar.txt, Qt::CaseInsensitive))
				{
					tariff_id = tar.id;
					break;
				}
			}
		}
		// 4. проверяю столбец "модель шины" начинается txt
		if (tariff_id == 0)
		{
			foreach(TariffInfo tar, Tariffs)
			{
				if (tar.mode != 2)
					continue;
				if (str_tmpl.startsWith(tar.txt, Qt::CaseInsensitive))
				{
					tariff_id = tar.id;
					break;
				}
			}
		}
		// 5. проверяю столбец "модель шины" содержит txt
		if (tariff_id == 0)
		{
			foreach(TariffInfo tar, Tariffs)
			{
				if (tar.mode != 1)
					continue;
				if (str_tmpl.contains(tar.txt, Qt::CaseInsensitive))
				{
					tariff_id = tar.id;
					break;
				}
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
			.arg(row[columnMap[IMPORT_TAG_NUM]].toInt())
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

	double sum = 0;

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
			QString str_fio2 = str_fio;
			QStringList items_fio = str_fio2.simplified().split(" ");
			if (items_fio.size() > 2)
				str_fio2 = items_fio[0] + " " + items_fio[1];

			str_query = QString("SELECT id FROM fio WHERE name='%1'").arg(str_fio2);
			if (!query.exec(str_query) || !query.next())
			{
				bool ok;
				str_fio2 = QInputDialog::getItem(NULL, QString("Внимание!"),
					QString("В справочнике 'Люди' не найдена запись: '%1'.\nНеобходимо выбрать из существующих:").arg(str_fio), l_fio, 0, false, &ok, Qt::MSWindowsFixedSizeDialogHint);
				if (!ok || str_fio2.isEmpty())
				{
					ZMessager::Instance().Message(_CriticalError, QString("В справочнике 'Люди' не найдена запсись: '%1'").arg(str_fio));
					continue;
				}
				
				str_query = QString("SELECT id FROM fio WHERE name='%1'").arg(str_fio2);
				if (!query.exec(str_query) || !query.next())
				{
					ZMessager::Instance().Message(_CriticalError, QString("В справочнике 'Люди' не найдена запсись: '%1'").arg(str_fio));
					continue;
				}
			}
		}
		fio = query.value(0).toInt();

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

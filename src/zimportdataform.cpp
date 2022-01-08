#include <QSqlQuery>
#include <QSqlError>
#include <QCompleter>
#include <QMessageBox>
#include "zimportdataform.h"

ZImportDataForm::ZImportDataForm(QWidget* parent, Qt::WindowFlags flags) : ZEditAbstractForm(parent, flags)
{
	ui.setupUi(this);
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(applyChanges()));
	
	ui.date->setDate(QDate::currentDate());

	ui.cboFIO->setEditable(true);
}

ZImportDataForm::~ZImportDataForm(){}

int ZImportDataForm::init(const QString &table, int id )
{
	ZEditAbstractForm::init(table, id);

	loadFio();

	QString stringQuery = QString("SELECT begin_dt, end_dt, fio, note FROM import_data WHERE id = %1")
		.arg(curEditId);

	// new record
	if (curEditId == ADD_UNIC_CODE)
	{
		ui.cboFIO->setCurrentIndex(0);
		return true;
	}

	// execute request
	QSqlQuery query;
	bool result = query.exec(stringQuery);
	if (result)
	{
		if (query.next()) 
		{
			int indx = query.value(4).toInt();

			ui.cboFIO->setCurrentIndex(ui.cboFIO->findData(query.value(2).toInt()));
			ui.date->setDate(query.value(0).toDate());
		}
	}	
	else 
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
	}

	return result;
}

void ZImportDataForm::loadFio()
{
	ui.cboFIO->clear();

	QSqlQuery query;
	if (query.exec("SELECT id,name FROM fio ORDER BY name"))
	{
		while (query.next())
		{
			ui.cboFIO->addItem(query.value(1).toString(), query.value(0).toInt());
		}
	}
	else
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
	}

	QCompleter* completer = new QCompleter(this);
	completer->setModel(ui.cboFIO->model());
	ui.cboFIO->setCompleter(completer);
}

void ZImportDataForm::applyChanges()
{
	QString text, stringQuery;

	if (curEditId == ADD_UNIC_CODE)
		stringQuery = QString("INSERT INTO import_data (begin_dt, end_dt, fio, note) VALUES (?, ?, ?, ?)");
	else
		stringQuery = QString("UPDATE import_data SET begin_dt=?, end_dt=?, fio=?, note=? WHERE id=%1").arg(curEditId);

	QSqlQuery query;
	query.prepare(stringQuery);
	
	query.addBindValue(ui.date->date());
	query.addBindValue(ui.cboFIO->itemData(ui.cboFIO->findText(ui.cboFIO->currentText()), Qt::UserRole));

	if(!query.exec())
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
		return;
	}
	
	accept();
}

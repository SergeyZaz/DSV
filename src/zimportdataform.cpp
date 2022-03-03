#include <QSqlQuery>
#include <QSqlError>
#include <QCompleter>
#include "zimportdataform.h"
#include "zsettings.h"
#include "zmessager.h"

ZImportDataForm::ZImportDataForm(QWidget* parent, Qt::WindowFlags flags) : ZEditAbstractForm(parent, flags)
{
	ui.setupUi(this);
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(applyChanges()));
	connect(ui.cmdSaveNew, SIGNAL(clicked()), this, SLOT(addNewSlot()));

	ui.date->setDate(QDate::currentDate());
	if (ZSettings::Instance().m_CloseDate.isValid())
		ui.date->setMinimumDate(ZSettings::Instance().m_CloseDate.addDays(1));

	ui.cboFIO->setEditable(true);
	ui.cboTariff->setEditable(true);
	ui.cboSmena->setEditable(true);
	ui.spinNum->setMaximum(999999);
	ui.spinNum->setMinimum(0);
}

ZImportDataForm::~ZImportDataForm(){}

int ZImportDataForm::init(const QString &table, int id )
{
	ZEditAbstractForm::init(table, id);

	loadCbo(ui.cboFIO, "fio");
	loadCbo(ui.cboTariff, "tariff"); 
	loadCbo(ui.cboSmena, "smena");

	QString stringQuery = QString("SELECT dt, smena, tariff, fio, num FROM import_data WHERE id = %1")
		.arg(curEditId);

	// new record
	if (curEditId == ADD_UNIC_CODE)
	{
		ui.cboFIO->setCurrentIndex(0);
		ui.cboTariff->setCurrentIndex(0);
		ui.cboSmena->setCurrentIndex(0);
		ui.spinNum->setValue(0);
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

			QDate d = query.value(0).toDate();
			if (ZSettings::Instance().m_CloseDate.isValid() && d < ZSettings::Instance().m_CloseDate)
			{
				ZMessager::Instance().Message(_CriticalError, tr("Дата привязки меньше даты закрытия!"), tr("Редактирование запрещено"));
				return false;
			}

			ui.cboFIO->setCurrentIndex(ui.cboFIO->findData(query.value(3).toInt()));
			ui.cboTariff->setCurrentIndex(ui.cboTariff->findData(query.value(2).toInt()));
			ui.cboSmena->setCurrentIndex(ui.cboSmena->findData(query.value(1).toInt()));
			ui.date->setDate(d);
			ui.spinNum->setValue(query.value(4).toInt());
		}
	}	
	else 
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
	}

	return result;
}

void ZImportDataForm::loadCbo(QComboBox *cbo, QString tbl)
{
	cbo->clear();

	QSqlQuery query;
	if (query.exec(QString("SELECT id,%2 FROM %1 ORDER BY %2").arg(tbl).arg(tbl=="tariff" ? "txt" : "name")))
	{
		while (query.next())
		{
			cbo->addItem(query.value(1).toString(), query.value(0).toInt());
		}
	}
	else
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
	}

	QCompleter* completer = new QCompleter(this);
	completer->setModel(cbo->model());
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	cbo->setCompleter(completer);
}

void ZImportDataForm::addNewSlot()
{
	curEditId = ADD_UNIC_CODE;
	applyChanges();
}

void ZImportDataForm::applyChanges()
{
	QString text, stringQuery;

	if (curEditId == ADD_UNIC_CODE)
		stringQuery = QString("INSERT INTO import_data (dt, smena, tariff, fio, num) VALUES (?, ?, ?, ?, ?)");
	else
		stringQuery = QString("UPDATE import_data SET dt=?, smena=?, tariff=?, fio=?, num=? WHERE id=%1").arg(curEditId);

	QSqlQuery query;
	query.prepare(stringQuery);
	
	query.addBindValue(ui.date->date());
	query.addBindValue(ui.cboSmena->itemData(ui.cboSmena->findText(ui.cboSmena->currentText()), Qt::UserRole));
	query.addBindValue(ui.cboTariff->itemData(ui.cboTariff->findText(ui.cboTariff->currentText()), Qt::UserRole));
	query.addBindValue(ui.cboFIO->itemData(ui.cboFIO->findText(ui.cboFIO->currentText()), Qt::UserRole));
	query.addBindValue(ui.spinNum->value());

	if(!query.exec())
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
		return;
	}
	
	accept();
}

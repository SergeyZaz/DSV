#include <QSqlQuery>
#include <QSqlError>
#include <QCompleter>
#include <QMessageBox>
#include "znotesform.h"

ZNotesForm::ZNotesForm(QWidget* parent, Qt::WindowFlags flags) : ZEditAbstractForm(parent, flags)
{
	ui.setupUi(this);
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(applyChanges()));
	connect(ui.cmdSaveNew, SIGNAL(clicked()), this, SLOT(addNewSlot()));

	ui.dateStart->setDate(QDate::currentDate());
	ui.dateEnd->setDate(QDate::currentDate());

	ui.cboFIO->setEditable(true);
}

ZNotesForm::~ZNotesForm(){}

int ZNotesForm::init(const QString &table, int id )
{
	ZEditAbstractForm::init(table, id);

	loadFio();

	QString stringQuery = QString("SELECT begin_dt, end_dt, fio, note FROM notes2fio WHERE id = %1")
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
			ui.dateStart->setDate(query.value(0).toDate());
			ui.dateEnd->setDate(query.value(1).toDate());
			ui.txtComment->setText(query.value(3).toString());
		}
	}	
	else 
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
	}

	return result;
}

void ZNotesForm::loadFio()
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

void ZNotesForm::addNewSlot()
{
	curEditId = ADD_UNIC_CODE;
	applyChanges();
}

void ZNotesForm::applyChanges()
{
	QString text, stringQuery;

	if (curEditId == ADD_UNIC_CODE)
		stringQuery = QString("INSERT INTO notes2fio (begin_dt, end_dt, fio, note) VALUES (?, ?, ?, ?)");
	else
		stringQuery = QString("UPDATE notes2fio SET begin_dt=?, end_dt=?, fio=?, note=? WHERE id=%1").arg(curEditId);

	QSqlQuery query;
	query.prepare(stringQuery);
	
	query.addBindValue(ui.dateStart->date());
	query.addBindValue(ui.dateEnd->date());
	query.addBindValue(ui.cboFIO->itemData(ui.cboFIO->findText(ui.cboFIO->currentText()), Qt::UserRole));
	query.addBindValue(ui.txtComment->toPlainText());

	if(!query.exec())
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
		return;
	}
	
	accept();
}

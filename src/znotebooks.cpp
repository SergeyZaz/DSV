#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include "znotebooks.h"
#include "znotesform.h"

ZNotebooks::ZNotebooks(QWidget* parent, Qt::WindowFlags flags): ZMdiChild(parent, flags)
{
}

void ZNotebooks::init(const QString &m_TblName)
{
	QList<int> hideColumns;
	QStringList headers;
	QList<int> cRem;
	
	hideColumns << 0;
	headers << tr("id") << tr("Дата") << tr("Заметка") << tr("Примечание");

	m_tbl->setTable(m_TblName, headers, cRem);	
	m_tbl->setCustomEditor(new ZNotebooksForm(this));

	m_tbl->setResizeRowsToContents(true);
	m_tbl->setResizeColumnsToContents(true);

	m_tbl->init(hideColumns, 1, Qt::DescendingOrder);
}

/////////////////////////////////////////////////////////////////////////////////////////

ZNotebooksForm::ZNotebooksForm(QWidget* parent, Qt::WindowFlags flags) : ZEditAbstractForm(parent, flags)
{
	ui.setupUi(this);
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(applyChanges()));
	connect(ui.cmdSaveNew, SIGNAL(clicked()), this, SLOT(addNewSlot()));

	ui.date->setDate(QDate::currentDate());
}

ZNotebooksForm::~ZNotebooksForm() {}

int ZNotebooksForm::init(const QString& table, int id)
{
	ZEditAbstractForm::init(table, id);


	QString stringQuery = QString("SELECT dt, note, comment FROM notebook WHERE id = %1")
		.arg(curEditId);

	// new record
	if (curEditId == ADD_UNIC_CODE)
	{
		return true;
	}

	// execute request
	QSqlQuery query;
	bool result = query.exec(stringQuery);
	if (result)
	{
		if (query.next())
		{
			ui.date->setDate(query.value(0).toDate());
			ui.txtNote->setText(query.value(1).toString());
			ui.txtComment->setText(query.value(2).toString());
		}
	}
	else
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
	}

	return result;
}

void ZNotebooksForm::addNewSlot()
{
	curEditId = ADD_UNIC_CODE;
	applyChanges();
}

void ZNotebooksForm::applyChanges()
{
	QString text, stringQuery;

	if (curEditId == ADD_UNIC_CODE)
		stringQuery = QString("INSERT INTO notebook (dt, note, comment) VALUES (?, ?, ?)");
	else
		stringQuery = QString("UPDATE notebook SET dt=?, note=?, comment=? WHERE id=%1").arg(curEditId);

	QSqlQuery query;
	query.prepare(stringQuery);

	query.addBindValue(ui.date->date());
	query.addBindValue(ui.txtNote->toPlainText());
	query.addBindValue(ui.txtComment->toPlainText());

	if (!query.exec())
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
		return;
	}

	accept();
}

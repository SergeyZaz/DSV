#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include "zpaymentsform.h"

ZPaymentsForm::ZPaymentsForm(QWidget* parent, Qt::WindowFlags flags) : ZEditAbstractForm(parent, flags)
{
	ui.setupUi(this);
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(applyChanges()));
	
	ui.cboMode->addItem("выплата", 0);
	ui.cboMode->addItem("вычет", 1);
}

ZPaymentsForm::~ZPaymentsForm(){}

int ZPaymentsForm::init( QSqlDatabase &database, const QString &table, int id )
{
	ZEditAbstractForm::init(database, table, id);

	ui.txtName->setFocus();

	QString stringQuery = QString("SELECT name,mode,comment FROM payments WHERE id = %1")
		.arg(curEditId);

	// new record
	if (curEditId == ADD_UNIC_CODE)
	{
		ui.txtName->setText("");
		ui.txtComment->setText("");
		ui.cboMode->setCurrentIndex(0);
		return true;
	}

	// execute request
	QSqlQuery query(m_DB);
	bool result = query.exec(stringQuery);
	if (result)
	{
		if (query.next()) 
		{
			ui.txtName->setText(query.value(0).toString());
			ui.txtComment->setText(query.value(2).toString());
			ui.cboMode->setCurrentIndex(ui.cboMode->findData(query.value(1).toInt()));
		}
	}	
	else 
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
	}

	return result;
}

void ZPaymentsForm::applyChanges()
{
	QString text, stringQuery;

	if (curEditId == ADD_UNIC_CODE)
		stringQuery = QString("INSERT INTO payments (name,mode,comment) VALUES (?, ?, ?)");
	else
		stringQuery = QString("UPDATE payments SET name=?, mode=?, comment=? WHERE id=%1").arg(curEditId);

	QSqlQuery query(m_DB);
	query.prepare(stringQuery);

	text = ui.txtName->text();
	if(text.isEmpty())
	{
		QMessageBox::critical(this, tr("Ошибка"), tr("Не заполнено обязательное поле 'Краткое наименование'"));
		return;
	}
	query.addBindValue(text);
	
	query.addBindValue(ui.cboMode->itemData(ui.cboMode->currentIndex(), Qt::UserRole));

	text = ui.txtComment->toPlainText();
	if(text.isEmpty())
	{
		text = " ";
	}
	query.addBindValue(text);

	if(!query.exec())
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
		return;
	}
	
	accept();
}

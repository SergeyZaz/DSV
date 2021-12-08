#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include "zpayments2fioform.h"

ZPayments2FioForm::ZPayments2FioForm(QWidget* parent, Qt::WindowFlags flags) : ZEditAbstractForm(parent, flags)
{
	ui.setupUi(this);
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(applyChanges()));
	connect(ui.cboMode, SIGNAL(currentIndexChanged(int)), this, SLOT(changeMode(int)));

	ui.dateEdit->setDate(QDate::currentDate());
	ui.cboMode->addItem("выплата", 0);
	ui.cboMode->addItem("вычет", 1);
}

ZPayments2FioForm::~ZPayments2FioForm(){}

int ZPayments2FioForm::init( QSqlDatabase &database, const QString &table, int id )
{
	ZEditAbstractForm::init(database, table, id);

	loadFio();

	QString stringQuery = QString("SELECT payment,fio,dt,val,payments.mode FROM payments2fio INNER JOIN payments ON(payments.id = payment) WHERE payments2fio.id = %1")
		.arg(curEditId);

	// new record
	if (curEditId == ADD_UNIC_CODE)
	{
		ui.cboMode->setCurrentIndex(0);
		changeMode(0);

		ui.cboFIO->setCurrentIndex(0);
		ui.cboPayment->setCurrentIndex(0);
		ui.spinVal->setValue(0);
		return true;
	}

	// execute request
	QSqlQuery query(m_DB);
	bool result = query.exec(stringQuery);
	if (result)
	{
		if (query.next()) 
		{
			int indx = query.value(4).toInt();
			ui.cboMode->setCurrentIndex(ui.cboMode->findData(indx));
			changeMode(indx);

			ui.cboFIO->setCurrentIndex(ui.cboFIO->findData(query.value(1).toInt()));
			ui.cboPayment->setCurrentIndex(ui.cboPayment->findData(query.value(0).toInt()));
			ui.dateEdit->setDate(query.value(2).toDate());
			ui.spinVal->setValue(query.value(3).toDouble());
		}
	}	
	else 
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
	}

	return result;
}

void ZPayments2FioForm::loadFio()
{
	ui.cboFIO->clear();

	QSqlQuery query(m_DB);
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
}

void ZPayments2FioForm::changeMode(int indx)
{
	ui.lblPayment->setText((indx==0) ? "Выплата:" : "Вычет:");

	ui.cboPayment->clear();

	QSqlQuery query(m_DB);
	QString stringQuery = QString("SELECT id,name FROM payments WHERE mode = %1 ORDER BY name").arg(indx);
	if (query.exec(stringQuery))
	{
		while(query.next())
		{
			ui.cboPayment->addItem(query.value(1).toString(), query.value(0).toInt());
		}
	}
	else
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
	}
}

void ZPayments2FioForm::applyChanges()
{
	QString text, stringQuery;

	if (curEditId == ADD_UNIC_CODE)
		stringQuery = QString("INSERT INTO payments2fio (payment,fio,dt,val) VALUES (?, ?, ?, ?)");
	else
		stringQuery = QString("UPDATE payments2fio SET payment=?, fio=?, dt=?, val=? WHERE id=%1").arg(curEditId);

	QSqlQuery query(m_DB);
	query.prepare(stringQuery);
	
	query.addBindValue(ui.cboPayment->itemData(ui.cboMode->currentIndex(), Qt::UserRole));
	query.addBindValue(ui.cboFIO->itemData(ui.cboMode->currentIndex(), Qt::UserRole));
	query.addBindValue(ui.dateEdit->date());
	query.addBindValue(ui.spinVal->value());

	if(!query.exec())
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
		return;
	}
	
	accept();
}

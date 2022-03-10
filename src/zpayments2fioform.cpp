#include <QSqlQuery>
#include <QSqlError>
#include <QCompleter>
#include "zpayments2fioform.h"
#include "zsettings.h"
#include "zmessager.h"

ZPayments2FioForm::ZPayments2FioForm(QWidget* parent, Qt::WindowFlags flags) : ZEditAbstractForm(parent, flags)
{
	ui.setupUi(this);
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(applyChanges()));
	connect(ui.cmdSaveNew, SIGNAL(clicked()), this, SLOT(addNewSlot()));
	connect(ui.cboMode, SIGNAL(currentIndexChanged(int)), this, SLOT(changeMode(int)));

	ui.dateEdit->setDate(QDate::currentDate());
	ui.dateLinkEdit->setDate(QDate::currentDate());
	ui.cboMode->addItem("бонус", 0);
	ui.cboMode->addItem("вычет", 1);

	if (ZSettings::Instance().m_CloseDate.isValid())
		ui.dateLinkEdit->setMinimumDate(ZSettings::Instance().m_CloseDate.addDays(1));

	ui.cboFIO->setEditable(true);
}

ZPayments2FioForm::~ZPayments2FioForm(){}

int ZPayments2FioForm::init(const QString &table, int id )
{
	ZEditAbstractForm::init(table, id);

	loadFio();

	QString stringQuery = QString("SELECT payment,fio,dt,val,payments.mode,dt_link,payments2fio.comment FROM payments2fio INNER JOIN payments ON(payments.id = payment) WHERE payments2fio.id = %1")
		.arg(curEditId);

	// new record
	if (curEditId == ADD_UNIC_CODE)
	{
		ui.cboMode->setCurrentIndex(0);
		changeMode(0);

		ui.cboFIO->setCurrentIndex(0);
		ui.cboPayment->setCurrentIndex(0);
		ui.spinVal->setValue(0);
		ui.txtComment->setText("");
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
			ui.cboMode->setCurrentIndex(ui.cboMode->findData(indx));
			changeMode(indx);

			QDate d = query.value(5).toDate();
			if (ZSettings::Instance().m_CloseDate.isValid() && d < ZSettings::Instance().m_CloseDate)
			{
				ZMessager::Instance().Message(_CriticalError, tr("Дата привязки меньше даты закрытия!"), tr("Редактирование запрещено"));
				return false;
			}

			ui.cboFIO->setCurrentIndex(ui.cboFIO->findData(query.value(1).toInt()));
			ui.cboPayment->setCurrentIndex(ui.cboPayment->findData(query.value(0).toInt()));
			ui.dateEdit->setDate(query.value(2).toDate());
			ui.spinVal->setValue(query.value(3).toDouble());
			ui.dateLinkEdit->setDate(d);			
			ui.txtComment->setText(query.value(6).toString());
		}
	}	
	else 
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
	}

	return result;
}

void ZPayments2FioForm::loadFio()
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
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
	}

	QCompleter* completer = new QCompleter(this);
	completer->setModel(ui.cboFIO->model());
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	ui.cboFIO->setCompleter(completer);
}

void ZPayments2FioForm::changeMode(int indx)
{
	ui.lblPayment->setText((indx==0) ? "Выплата:" : "Вычет:");

	ui.cboPayment->clear();

	QSqlQuery query;
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
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
	}
}

void ZPayments2FioForm::addNewSlot()
{
	curEditId = ADD_UNIC_CODE;
	applyChanges();
}

void ZPayments2FioForm::applyChanges()
{
	QString text, stringQuery;

	if (curEditId == ADD_UNIC_CODE)
		stringQuery = QString("INSERT INTO payments2fio (payment,fio,dt,val,dt_link,comment) VALUES (?, ?, ?, ?, ?, ?)");
	else
		stringQuery = QString("UPDATE payments2fio SET payment=?, fio=?, dt=?, val=?, dt_link=?, comment=? WHERE id=%1").arg(curEditId);

	QSqlQuery query;
	query.prepare(stringQuery);
	
//	QString t = ui.cboFIO->currentText();
//	int i = ui.cboFIO->findText(t);
	query.addBindValue(ui.cboPayment->itemData(ui.cboPayment->currentIndex(), Qt::UserRole));
	query.addBindValue(ui.cboFIO->itemData(ui.cboFIO->findText(ui.cboFIO->currentText()), Qt::UserRole));
	query.addBindValue(ui.dateEdit->date());
	query.addBindValue(ui.spinVal->value());
	query.addBindValue(ui.dateLinkEdit->date());
	query.addBindValue(ui.txtComment->toPlainText());


	if(!query.exec())
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
		return;
	}
	
	accept();
}

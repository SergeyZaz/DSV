#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include "ztariffsform.h"

ZTariffsForm::ZTariffsForm(QWidget* parent, Qt::WindowFlags flags) : ZEditAbstractForm(parent, flags)
{
	ui.setupUi(this);
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(applyChanges()));
	
	ui.cboMode->addItem("вид работы", 0);
	ui.cboMode->addItem("модель шины содержит ...", 1);
	ui.cboMode->addItem("модель шины начинается с ...", 2);
	ui.cboMode->addItem("модель шины заканчивается ...", 3);
	ui.cboMode->addItem("Rate содержит ...", 4);

	ui.cboType->addItem("за смену", 0);
	ui.cboType->addItem("за штуку", 1);

}

ZTariffsForm::~ZTariffsForm(){}

int ZTariffsForm::init( QSqlDatabase &database, const QString &table, int id )
{
	ZEditAbstractForm::init(database, table, id);

	ui.txtName->setFocus();

	QString stringQuery = QString("SELECT txt,mode,bonus,type,comment FROM tariff WHERE id = %1")
		.arg(curEditId);

	// new record
	if (curEditId == ADD_UNIC_CODE)
	{
		ui.txtName->setText("");
		ui.txtComment->setText("");
		ui.spinBonus->setValue(0);
		ui.cboMode->setCurrentIndex(0);
		ui.cboType->setCurrentIndex(0);
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
			ui.txtComment->setText(query.value(4).toString());
			ui.spinBonus->setValue(query.value(2).toDouble());
			ui.cboMode->setCurrentIndex(ui.cboMode->findData(query.value(1).toInt()));
			ui.cboType->setCurrentIndex(ui.cboType->findData(query.value(3).toInt()));
		}
	}	
	else 
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
	}

	return result;
}

void ZTariffsForm::applyChanges()
{
	QString text, stringQuery;

	if (curEditId == ADD_UNIC_CODE)
		stringQuery = QString("INSERT INTO tariff (txt,mode,bonus,type,comment) VALUES (?, ?, ?, ?, ?)");
	else
		stringQuery = QString("UPDATE tariff SET txt=?, mode=?, bonus=?, type=?, comment=? WHERE id=%1").arg(curEditId);

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

	query.addBindValue(ui.spinBonus->value());
	
	query.addBindValue(ui.cboType->itemData(ui.cboType->currentIndex(), Qt::UserRole));

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

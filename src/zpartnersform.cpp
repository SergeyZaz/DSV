#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include "zpartnersform.h"

ZPartnersForm::ZPartnersForm()
{
	ui.setupUi(this);
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(applyChanges()));
	connect(ui.cmdAddSectionPlus, SIGNAL(clicked()), this, SLOT(addAddSection()));
	connect(ui.cmdAddSectionMinus, SIGNAL(clicked()), this, SLOT(addAddSection()));
	showEvent(NULL);
}

ZPartnersForm::~ZPartnersForm(){}

void ZPartnersForm::showEvent(QShowEvent*)
{
	loadItemsToCombobox(ui.cboSectionPlus, "sections", "type=0");
	loadItemsToCombobox(ui.cboSectionMinus, "sections", "type=1");
}

int ZPartnersForm::init( QSqlDatabase &database, const QString &table, int id )
{
	ZEditAbstractForm::init(database, table, id);

	ui.txtName->setFocus();

	QString stringQuery = QString("SELECT name,full_name,comment,inn,kpp,section_plus,section_minus FROM partners WHERE id = %1")
		.arg(curEditId);

	// new record
	if (curEditId == ADD_UNIC_CODE)
	{
		ui.txtName->setText("");
		ui.txtNameFull->setText("");
		ui.txtComment->setText("");
		ui.txtINN->setText("");
		ui.txtKPP->setText("");
		ui.cboSectionPlus->setCurrentIndex(0);
		ui.cboSectionMinus->setCurrentIndex(0);
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
			ui.txtNameFull->setText(query.value(1).toString());
			ui.txtComment->setText(query.value(2).toString());
			ui.txtINN->setText(query.value(3).toString());
			ui.txtKPP->setText(query.value(4).toString());

			ui.ckbDefault->setChecked((query.value(5).toInt() == 0) && (query.value(6).toInt() == -1));
			if(ui.ckbDefault->isChecked())
			{
				ui.cboSectionPlus->setCurrentIndex(0);
				ui.cboSectionMinus->setCurrentIndex(0);
			}
			else
			{
				ui.cboSectionPlus->setCurrentIndex(ui.cboSectionPlus->findData(query.value(5).toInt()));
				ui.cboSectionMinus->setCurrentIndex(ui.cboSectionMinus->findData(query.value(6).toInt()));
			}
		}
	}	
	else 
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
	}

	return result;
}

void ZPartnersForm::applyChanges()
{
	QString text, stringQuery;

	if (curEditId == ADD_UNIC_CODE)
		stringQuery = QString("INSERT INTO partners ( name,full_name,comment,inn,kpp,section_plus,section_minus ) VALUES (?, ?, ?, ?, ?, ?, ?)");
	else
		stringQuery = QString("UPDATE partners SET name=?, full_name=?, comment=?, inn=?, kpp=?, section_plus=?, section_minus=? WHERE id=%1").arg(curEditId);

	QSqlQuery query(m_DB);
	query.prepare(stringQuery);

	text = ui.txtName->text();
	if(text.isEmpty())
	{
		QMessageBox::critical(this, tr("Ошибка"), tr("Не заполнено обязательное поле 'Краткое наименование'"));
		return;
	}
	query.addBindValue(text);

	text = ui.txtNameFull->text();
	if(text.isEmpty())
	{
		text = " ";
	}
	query.addBindValue(text);

	text = ui.txtComment->toPlainText();
	if(text.isEmpty())
	{
		text = " ";
	}
	query.addBindValue(text);

	text = ui.txtINN->text();
	if(text.isEmpty())
	{
		QMessageBox::critical(this, tr("Ошибка"), tr("Не заполнено обязательное поле 'ИНН'"));
		return;
	}
	query.addBindValue(text);

	text = ui.txtKPP->text();
	if(text.isEmpty())
	{
		text = " ";
	}
	query.addBindValue(text);

	if(ui.ckbDefault->isChecked())
	{
		query.addBindValue(0);
		query.addBindValue(-1);
	}
	else
	{
		query.addBindValue(ui.cboSectionPlus->itemData(ui.cboSectionPlus->currentIndex(), Qt::UserRole));
		query.addBindValue(ui.cboSectionMinus->itemData(ui.cboSectionMinus->currentIndex(), Qt::UserRole));
	}

	if(!query.exec())
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
		return;
	}
	
	accept();
}

void ZPartnersForm::addAddSection()
{
//	ZEditAbstractForm *pD = new ZSectionsForm;
//	pD->init(m_DB, "sections", ADD_UNIC_CODE);
//	pD->exec();
//	showEvent(NULL);
}

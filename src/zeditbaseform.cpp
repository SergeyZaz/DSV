#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include "zeditbaseform.h"

ZEditBaseForm::ZEditBaseForm(QWidget*, Qt::WindowFlags)
{
	ui.setupUi(this);
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(applySlot()));
	connect(ui.cmdSaveNew, SIGNAL(clicked()), this, SLOT(addNewSlot()));
	connect(ui.txtName, SIGNAL(textChanged(const QString &)), this, SLOT(textChangedSlot(const QString &)));
	textChangedSlot("");
}

ZEditBaseForm::~ZEditBaseForm()
{
}

void ZEditBaseForm::closeEvent(QCloseEvent *)
{
}

int ZEditBaseForm::init(const QString &tbl, int id)
{
	ZEditAbstractForm::init(tbl, id);

	fNeedComment = true;

	if(m_tbl == "sbk.rank" ||
		m_tbl == "sbk.deptpost" ||
		m_tbl == "sbk.brcategory" ||
		m_tbl == "sbk.arealvl" )
			fNeedComment = false;

	QString sQry;

	if(!fNeedComment)
	{
		ui.pnlComment->hide();
		sQry = QString("SELECT name FROM %1 WHERE id=%2")
			.arg(m_tbl)
			.arg(curEditId);
	}
	else
	{
		sQry = QString("SELECT name, comment FROM %1 WHERE id=%2")
			.arg(m_tbl)
			.arg(curEditId);
	}

	ui.txtComment->setText("");
	ui.txtName->setText("");

	if(curEditId==ADD_UNIC_CODE)
		return 1;

	
	QStringList list;
	QSqlQuery m_Query;
	int rc = m_Query.exec(sQry);
	if(rc)
	{
		while (m_Query.next()) 
		{ 
			int sch = 0;
			QVariant value = m_Query.value(0);
			while (value.isValid()) 
			{ 
				list.push_back(value.toString());
				sch++;
				value = m_Query.value(sch); 
			}
			break;
		}
	}
	else
	{
		emit errorQuery(QDateTime::currentDateTime(), m_Query.lastError().number(), m_Query.lastError().text());
		return 0;
	}

	if(list.size()>0)
		ui.txtName->setText(list[0]);
	
	if(fNeedComment && list.size()>1)
		ui.txtComment->setText(list[1]);

	return 1;
}

void ZEditBaseForm::textChangedSlot(const QString &text)
{
	bool fOK = !text.isEmpty();
	// отображение статуса высоты окна
	ui.nameStatus->setPixmap( (fOK) ? QPixmap( ":/qresource/images/correct.png" ) : QPixmap( ":/qresource/images/incorrect.png" ) );
	ui.cmdSave->setEnabled(fOK);
}

void ZEditBaseForm::addNewSlot()
{
	curEditId = ADD_UNIC_CODE;
	if (applyChange() == 1)
		accept();
}

void ZEditBaseForm::applySlot()
{
	if(applyChange()==1)
		accept();
}

int ZEditBaseForm::applyChange()
{
	QString sQry;
	QString tName = ui.txtName->text();
	QString tComm = ui.txtComment->toPlainText();

	if(curEditId==ADD_UNIC_CODE)
	{
		if(fNeedComment)
			sQry = QString("INSERT INTO %1(id, \"name\", \"comment\") VALUES(nextval('%1_id_seq'), '%2', '%3')")
				.arg(m_tbl)
				.arg(tName)
				.arg(tComm);
		else
			sQry = QString("INSERT INTO %1(id, \"name\") VALUES(nextval('%1_id_seq'), '%2')")
				.arg(m_tbl)
				.arg(tName);
	}
	else
	{
		if(fNeedComment)
			sQry = QString("UPDATE %1 SET \"name\"='%2', \"comment\"='%3' WHERE id=%4")
				.arg(m_tbl)
				.arg(tName)
				.arg(tComm)
				.arg(curEditId);
		else
			sQry = QString("UPDATE %1 SET \"name\"='%2' WHERE id=%3")
				.arg(m_tbl)
				.arg(tName)
				.arg(curEditId);
	}


	QSqlQuery m_Query;
	int rc = m_Query.exec(sQry);
	if(!rc)
	{
		emit errorQuery(QDateTime::currentDateTime(), m_Query.lastError().number(), m_Query.lastError().text());
		return 0;
	}

	return 1;
}


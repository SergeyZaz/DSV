#include "zconfigform.h"
#include "zmessager.h"
#include "zsettings.h"
#include "zview.h"

#include <QSqlQuery>
#include <QSqlError>

ZConfigForm::ZConfigForm(QWidget* parent, Qt::WindowFlags flags)
{
	ui.setupUi(this);
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(applyChanges()));

	ui.dateEdit->setDate(QDate::currentDate());

	if(GetCloseDate())
		ui.dateEdit->setDate(ZSettings::Instance().m_CloseDate);
}

ZConfigForm::~ZConfigForm() {}

void ZConfigForm::applyChanges()
{
	QSqlQuery query;
	if (!query.exec("DELETE FROM config WHERE key = 'closeDate'"))
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
		return;
	}
	
	ZSettings::Instance().m_CloseDate = ui.dateEdit->date();

	if (!query.exec(QString("INSERT INTO config(key, value) VALUES('closeDate','%1')").arg(ZSettings::Instance().m_CloseDate.toString(DATE_FORMAT))))
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), tr("Ошибка"));
		return;
	}

	accept();
}

bool GetCloseDate()
{
	QSqlQuery query;
	if (!query.exec("SELECT value FROM config WHERE key = 'closeDate'"))
	{
		ZMessager::Instance().Message(_CriticalError, query.lastError().text(), "Ошибка");
		return false;
	}

	if (!query.next())
		return false;

	ZSettings::Instance().m_CloseDate = query.value(0).toDate();
	return true;
}
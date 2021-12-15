#include "zorganisations.h"


ZOrganisations::ZOrganisations(QWidget* parent, Qt::WindowFlags flags)// : ZViewGroups(parent, flags)
{
}

void ZOrganisations::init(const QString &m_TblName)
{
	setup();

	QList<int> hideColumns;
	QStringList headers;
	QList<int> cRem;
	
	hideColumns << 0;
	headers <<  tr("id") << tr("Название") << tr("Комментарий");

	ui.m_tbl->setTable(m_TblName, headers, cRem);
	ui.m_tbl->init(hideColumns);

	setLinkTableName("organisation2fio");
}


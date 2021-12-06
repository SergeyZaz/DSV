#include "zgroups.h"

ZGroups::ZGroups(QWidget* parent, Qt::WindowFlags flags) : ZViewGroups(parent, flags)
{
}


void ZGroups::initDB(QSqlDatabase &m_DB, const QString &m_TblName)
{
	setDatabase(m_DB);

	QList<int> hideColumns;
	QStringList headers;
	QList<int> cRem;
	
	hideColumns << 0;
	headers <<  tr("id") << tr("Название") << tr("Комментарий");

	ui.m_tbl->setTable(m_TblName, headers, cRem);
	ui.m_tbl->init(hideColumns);

	setLinkTableName("groups2fio");
}


#include "zpersons.h"

ZPersons::ZPersons(QWidget* parent, Qt::WindowFlags flags) : ZMdiChild(parent, flags)
{
}


void ZPersons::initDB(QSqlDatabase &m_DB, const QString &m_TblName)
{
	m_tbl->setDatabase(m_DB);

	QList<int> hideColumns;
	QStringList headers;
	QList<int> cRem;
	
	hideColumns << 0;
	headers <<  tr("id") << tr("ФИО") << tr("Комментарий");

	m_tbl->setTable(m_TblName, headers, cRem);
	m_tbl->init(hideColumns);
	m_tbl->moveSection(3, 5);
}


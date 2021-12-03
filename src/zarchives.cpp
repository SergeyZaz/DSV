#include "zarchives.h"


ZArchives::ZArchives()
{
}

void ZArchives::initDB(QSqlDatabase& m_DB, const QString& m_TblName)
{
	m_tbl->setDatabase(m_DB);

	QList<int> hideColumns;
	QStringList headers;
	QList<int> cRem;

	hideColumns << 0;
	headers << tr("id") << tr("Имя файла") << tr("Дата/время") << tr("Статус");

	m_tbl->setTable(m_TblName, headers, cRem);

	QMap<int, QString>* pMap0 = new QMap<int, QString>;
	pMap0->insert(0, "не загружено");
	pMap0->insert(1, "загружено");
	m_tbl->setRelation(3, pMap0);

	m_tbl->init(hideColumns);
	m_tbl->moveSection(1, 2);

}
#include "zarchives.h"


ZArchives::ZArchives(QWidget* parent, Qt::WindowFlags flags): ZMdiChild(parent, flags)
{
}

void ZArchives::init(const QString& m_TblName)
{

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

	m_tbl->setReadOnly(true, true, false);
}
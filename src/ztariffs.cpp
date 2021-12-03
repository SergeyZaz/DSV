#include "ztariffs.h"
//#include "zpartnersform.h"

ZTariffs::ZTariffs()
{
}


void ZTariffs::initDB(QSqlDatabase &m_DB, const QString &m_TblName)
{
	m_tbl->setDatabase(m_DB);

	QList<int> hideColumns;
	QStringList headers;
	QList<int> cRem;
	QSqlQuery query;
	
	hideColumns << 0;
	headers <<  tr("id") << tr("Название") << tr("Режим") << tr("Доплата");
	
	m_tbl->setTable(m_TblName, headers, cRem);
	//m_tbl->setCustomEditor(new ZPartnersForm);

	//столбец "станок" (вид работы), 1 - столбец "модель шины" содержит txt, 2 - столбец "модель шины" начинается с txt, 3 - столбец "модель шины" заканчивается txt, 4 - столбец "Rate" значение txt
	QMap<int, QString> *pMap0 = new QMap<int, QString>;
	pMap0->insert(0, "вид работы");
	pMap0->insert(1, "модель шины содержит ...");
	pMap0->insert(2, "модель шины начинается с ...");
	pMap0->insert(3, "модель шины заканчивается ...");
	pMap0->insert(4, "Rate содержит ...");
	m_tbl->setRelation(2, pMap0);
	
	m_tbl->init(hideColumns);
}


#include "zpayments.h"
#include "zpaymentsform.h"


ZPayments::ZPayments(QWidget* parent, Qt::WindowFlags flags) : ZMdiChild(parent, flags)
{
}

void ZPayments::initDB(QSqlDatabase& m_DB, const QString& m_TblName)
{
	m_tbl->setDatabase(m_DB);

	QList<int> hideColumns;
	QStringList headers;
	QList<int> cRem;

	hideColumns << 0;
	headers << tr("id") << tr("Выплата") << tr("Комментарий") << tr("Тип");

	m_tbl->setTable(m_TblName, headers, cRem);
	m_tbl->setCustomEditor(new ZPaymentsForm(this));

	QMap<int, QString>* pMap1 = new QMap<int, QString>;
	pMap1->insert(0, "выплата");
	pMap1->insert(1, "вычет");
	m_tbl->setRelation(3, pMap1);

	m_tbl->init(hideColumns);
	m_tbl->moveSection(2, 3);
}
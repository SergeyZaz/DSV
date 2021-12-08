#include "zpayments2fio.h"
#include "zpayments2fioform.h"


ZPayments2fio::ZPayments2fio(QWidget* parent, Qt::WindowFlags flags) : ZMdiChild(parent, flags)
{
}

void ZPayments2fio::initDB(QSqlDatabase& m_DB, const QString& m_TblName)
{
	m_tbl->setDatabase(m_DB);

	QList<int> hideColumns;
	QStringList headers;
	QList<int> cRem;

	hideColumns << 0;
	headers << tr("id") << tr("Дата") << tr("ФИО") << tr("Выплата") << tr("Значение");

	m_tbl->setTable(m_TblName, headers, cRem);
	m_tbl->setCustomEditor(new ZPayments2FioForm(this));
	
	m_tbl->setRelation(2, "fio", "id", "name");
	m_tbl->setRelation(3, "payments", "id", "name");

	m_tbl->init(hideColumns);
	m_tbl->changeFilter(1);

	m_tbl->getTable()->setColumnWidth(1, 100);
	m_tbl->getTable()->setColumnWidth(2, 300);
	m_tbl->getTable()->setColumnWidth(3, 200);

}
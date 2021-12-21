#include "zpayments2fio.h"
#include "zpayments2fioform.h"


ZPayments2fio::ZPayments2fio(QWidget* parent, Qt::WindowFlags flags) : ZMdiChild(parent, flags)
{
}

void ZPayments2fio::init(const QString& m_TblName)
{
	QList<int> hideColumns;
	QStringList headers;
//	QList<int> cRem;

	hideColumns << 0 << 2 << 3 << 8;
	headers << tr("id") << tr("Дата") << tr("fio_id") << tr("payment_id") << tr("ФИО") << tr("Организация") << tr("Выплата") << tr("Значение") << tr("mode");

	m_tbl->setTable(m_TblName);

	m_tbl->setQuery("SELECT p.id, p.dt, p.fio, p.payment, fio.name, payments.name, organisation.name, p.val, payments.mode  FROM payments2fio AS p \
INNER JOIN fio ON(p.fio = fio.id) \
INNER JOIN payments ON(p.payment = payments.id) \
LEFT JOIN organisation2fio ON(p.fio = value) \
LEFT JOIN organisation ON(organisation2fio.key = organisation.id)", headers);
	m_tbl->setCustomEditor(new ZPayments2FioForm(this));
	
//	m_tbl->setRelation(2, "fio", "id", "name");
//	m_tbl->setRelation(3, "payments", "id", "name");

	m_tbl->init(hideColumns);
	m_tbl->changeFilter(1);

	m_tbl->getTblView()->setColumnWidth(1, 100);
	m_tbl->getTblView()->setColumnWidth(4, 300);
	m_tbl->getTblView()->setColumnWidth(5, 200);

	m_tbl->setColorHighligthIfColumnContain(8, 0, QColor(128, 255, 128));
	m_tbl->setColorHighligthIfColumnContain(8, 1, QColor(255, 128, 128));

	m_tbl->setReadOnly(false, false, false);

}
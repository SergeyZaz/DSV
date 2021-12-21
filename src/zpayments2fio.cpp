#include "zpayments2fio.h"
#include "zpayments2fioform.h"


ZPayments2fio::ZPayments2fio(QWidget* parent, Qt::WindowFlags flags) : QWidget(parent, flags)
{
	filterOrganisationId = -1;
	ui.setupUi(this);
	
	loadItemsToComboBox(ui.cboFilter, "organisation");
//	ui.date->setDate(QDate::currentDate());

	connect(ui.m_tbl, SIGNAL(needUpdateVal(int)), this, SLOT(UpdateSumma(int)));
//	connect(ui.date, SIGNAL(dateChanged(const QDate&)), this, SLOT(dateChangedSlot(const QDate&)));
	
	connect(ui.cboFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index)
		{
			filterOrganisationId = ui.cboFilter->currentData().toInt();
			ChangeFilter();
		});

}

void ZPayments2fio::ChangeFilter()
{
	QList<int> hideColumns;
	QStringList headers;

	hideColumns << 0 << 2 << 3 << 8;
	headers << tr("id") << tr("Дата") << tr("fio_id") << tr("payment_id") << tr("ФИО") << tr("Организация") << tr("Выплата") << tr("Значение") << tr("mode");

	QString query = "SELECT p.id, p.dt, p.fio, p.payment, fio.name, organisation.name, payments.name, p.val, payments.mode  FROM payments2fio AS p \
INNER JOIN fio ON(p.fio = fio.id) \
INNER JOIN payments ON(p.payment = payments.id) \
LEFT JOIN organisation2fio ON(p.fio = value) \
LEFT JOIN organisation ON(organisation2fio.key = organisation.id)";

	if (filterOrganisationId > 0)
			query += QString(" WHERE organisation.id = %1").arg(filterOrganisationId);

	ui.m_tbl->setQuery(query, headers);

	ui.m_tbl->init(hideColumns);

	ui.m_tbl->setReadOnly(false, false, false);

	ui.m_tbl->setColorHighligthIfColumnContain(8, 0, QColor(128, 255, 128));
	ui.m_tbl->setColorHighligthIfColumnContain(8, 1, QColor(255, 128, 128));

	UpdateSumma();
}

void ZPayments2fio::dateChangedSlot(const QDate &date)
{
	filterDate = date;
	ChangeFilter();
}

void ZPayments2fio::init(const QString& m_TblName)
{
	ui.m_tbl->setTable(m_TblName);
	ui.m_tbl->setCustomEditor(new ZPayments2FioForm(this));

	//	m_tbl->setRelation(2, "fio", "id", "name");
	//	m_tbl->setRelation(3, "payments", "id", "name");

	ChangeFilter();

	ui.m_tbl->changeFilter(1);

//	ui.m_tbl->getTblView()->setColumnWidth(1, 100);
//	ui.m_tbl->getTblView()->setColumnWidth(4, 300);
//b	ui.m_tbl->getTblView()->setColumnWidth(5, 200);
}

void ZPayments2fio::UpdateSumma(int)
{
	QString s;
	double summa = 0;
	int i, n = ui.m_tbl->getSortModel()->rowCount();
	for (i = 0; i < n; i++)
	{
		s = ui.m_tbl->getSortModel()->data(ui.m_tbl->getSortModel()->index(i, 7)).toString();
		if(ui.m_tbl->getSortModel()->data(ui.m_tbl->getSortModel()->index(i, 8)).toInt())
			summa -= QString2Double(s);
		else
			summa += QString2Double(s);
	}
	ui.lblSumma->setText(QString("Сумма: %L1").arg(summa, 0, 'f', 2));
}

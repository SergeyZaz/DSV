#include "zpayments2fio.h"
#include "zpayments2fioform.h"
#include "zparsexlsxfile.h"
#include "zsettings.h"
#include <QFileDialog>


ZPayments2fio::ZPayments2fio(QWidget* parent, Qt::WindowFlags flags) : QWidget(parent, flags)
{
	filterOrganisationId = -1;
	filterPaymentId = -1;
	ui.setupUi(this);
	
	loadItemsToComboBox(ui.cboFilter, "organisation");

	loadItemsToComboBox(ui.cboFilter2, "payments");
	ui.cboFilter2->insertItem(0, "не задано", -1);
	ui.cboFilter2->setCurrentIndex(0);
	//	ui.date->setDate(QDate::currentDate());

	connect(ui.m_tbl, SIGNAL(needUpdateVal(int)), this, SLOT(UpdateSumma(int)));
//	connect(ui.date, SIGNAL(dateChanged(const QDate&)), this, SLOT(dateChangedSlot(const QDate&)));
	connect(ui.cmdImport, SIGNAL(clicked()), this, SLOT(ImportSlot()));

	connect(ui.cboFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index)
		{
			filterOrganisationId = ui.cboFilter->currentData().toInt();
			ChangeFilter();
		});
	connect(ui.cboFilter2, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index)
		{
			filterPaymentId = ui.cboFilter2->currentData().toInt();
			ChangeFilter();
		});
}

void ZPayments2fio::ChangeFilter()
{
	QList<int> hideColumns;
	QStringList headers;

	hideColumns << 0 << 2 << 3 << 8;
	headers << tr("id") << tr("Дата") << tr("fio_id") << tr("payment_id") << tr("ФИО") << tr("Организация") << tr("Выплата") << tr("Значение") << tr("mode") << tr("Дата привязки");

	QString query = "SELECT p.id, p.dt, p.fio, p.payment, fio.name, organisation.name, payments.name, p.val, payments.mode, p.dt_link  FROM payments2fio AS p \
INNER JOIN fio ON(p.fio = fio.id) \
INNER JOIN payments ON(p.payment = payments.id) \
LEFT JOIN organisation2fio ON(p.fio = value) \
LEFT JOIN organisation ON(organisation2fio.key = organisation.id)";

	if (filterOrganisationId > 0 || filterPaymentId >= 0)
		query += QString(" WHERE ");
	if (filterOrganisationId > 0)
	{
		query += QString("organisation.id = %1").arg(filterOrganisationId);
		if (filterPaymentId >= 0)
			query += QString(" AND ");
	}
	if (filterPaymentId >= 0)
		query += QString("payments.id = %1").arg(filterPaymentId);

	ui.m_tbl->setQuery(query, headers);

	ui.m_tbl->init(hideColumns);

	ui.m_tbl->setReadOnly(false, false, false);

	ui.m_tbl->setColorHighligthIfColumnContain(8, 0, QColor(128, 255, 128));
	ui.m_tbl->setColorHighligthIfColumnContain(8, 1, QColor(255, 128, 128));
	
	connect(ui.m_tbl->getTblView()->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
		this, SLOT(SelectionChanged(const QItemSelection&, const QItemSelection&)));


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

void ZPayments2fio::ImportSlot()
{
	QString fileName = QFileDialog::getOpenFileName(this, "Выбор файла для импорта", "", "XLSX-файлы (*.xlsx);;Все файлы (*.*)");
	if (fileName.isEmpty())
		return;
	ZParseXLSXFile pFile;
	if (pFile.loadPayments(fileName))
	{
		ChangeFilter();
	}
}

void ZPayments2fio::SelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	QModelIndexList indxs = ui.m_tbl->getTblView()->selectionModel()->selectedIndexes();
	if (indxs.size() == 0)
		return;

	bool fEdit = true;
	foreach(auto indx, indxs)
	{
		QDate d = indx.model()->data(indx.model()->index(indx.row(), 9)).toDate();
		if (ZSettings::Instance().m_CloseDate.isValid() && d < ZSettings::Instance().m_CloseDate)
			fEdit = false;
	}
	ui.m_tbl->setReadOnly(!fEdit, false, !fEdit);
}

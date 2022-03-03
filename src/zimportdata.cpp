#include "zimportdata.h"
#include "zimportdataform.h"
#include "zsettings.h"

ZImportData::ZImportData(QWidget* parent, Qt::WindowFlags flags): ZMdiChild(parent, flags)
{
}

void ZImportData::init(const QString &m_TblName)
{
	QList<int> hideColumns;
	QStringList headers;
	QList<int> cRem;
	
	hideColumns << 0;
	headers <<  tr("id") << tr("Дата") << tr("Смена") << tr("Тариф") << tr("ФИО") << tr("Количество") << tr("Файл") << tr("Номер строки");

	m_tbl->setTable(m_TblName, headers, cRem);	
	m_tbl->setCustomEditor(new ZImportDataForm(this));

	m_tbl->setRelation(2, "smena", "id", "name");
	m_tbl->setRelation(3, "tariff", "id", "txt");
	m_tbl->setRelation(4, "fio", "id", "name");
	m_tbl->setRelation(6, "import", "id", "file");

	m_tbl->init(hideColumns);

	m_tbl->moveSection(7, 1);

//	m_tbl->setReadOnly(true, true, false);

//	m_tbl->getTblView()->verticalHeader()->setVisible(true);

	connect(m_tbl->getTblView()->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
		this, SLOT(SelectionChanged(const QItemSelection&, const QItemSelection&)));

}

void ZImportData::SelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	QModelIndexList indxs = m_tbl->getTblView()->selectionModel()->selectedIndexes();
	if (indxs.size() == 0)
		return;

	bool fEdit = true;
	foreach(auto indx, indxs)
	{
		QDate d = indx.model()->data(indx.model()->index(indx.row(), 1)).toDate();
		if (ZSettings::Instance().m_CloseDate.isValid() && d < ZSettings::Instance().m_CloseDate)
			fEdit = false;
	}
	m_tbl->setReadOnly(!fEdit, false, !fEdit);
}

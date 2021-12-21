#include "zimportdata.h"

ZImportData::ZImportData(QWidget* parent, Qt::WindowFlags flags): ZMdiChild(parent, flags)
{
}

void ZImportData::init(const QString &m_TblName)
{
	QList<int> hideColumns;
	QStringList headers;
	QList<int> cRem;
	
	hideColumns << 0;
	headers <<  tr("id") << tr("Дата") << tr("Смена") << tr("Тариф") << tr("ФИО") << tr("Количество") << tr("Файл");

	m_tbl->setTable(m_TblName, headers, cRem);	

	m_tbl->setRelation(2, "smena", "id", "name");
	m_tbl->setRelation(3, "tariff", "id", "txt");
	m_tbl->setRelation(4, "fio", "id", "name");
	m_tbl->setRelation(6, "import", "id", "file");

	m_tbl->init(hideColumns);

	m_tbl->setReadOnly(true, true, false);

	m_tbl->getTblView()->verticalHeader()->setVisible(true);
}


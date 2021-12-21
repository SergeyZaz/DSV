#include "znotes.h"
#include "znotesform.h"

ZNotes::ZNotes(QWidget* parent, Qt::WindowFlags flags): ZMdiChild(parent, flags)
{
}

void ZNotes::init(const QString &m_TblName)
{
	QList<int> hideColumns;
	QStringList headers;
	QList<int> cRem;
	
	hideColumns << 0;
	headers <<  tr("id") << tr("Начало") << tr("Окончание") << tr("ФИО") << tr("Заметка");

	m_tbl->setTable(m_TblName, headers, cRem);	
	m_tbl->setCustomEditor(new ZNotesForm(this));


	m_tbl->setRelation(3, "fio", "id", "name");

	m_tbl->init(hideColumns);

	m_tbl->moveSection(3, 1);
}


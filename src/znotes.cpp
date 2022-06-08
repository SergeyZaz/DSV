#include "znotes.h"
#include "znotesform.h"

ZNotes::ZNotes(QWidget* parent, Qt::WindowFlags flags): QWidget(parent, flags)
{
	ui.setupUi(this);

//	ui.date_begin->setDate(QDate::currentDate().addMonths(-1));
//	ui.date_end->setDate(QDate::currentDate());

	beginDate = ui.date_begin->date();
	endDate = ui.date_end->date();

	connect(ui.date_begin, SIGNAL(dateChanged(const QDate&)), this, SLOT(dateChangedSlot(const QDate&)));
	connect(ui.date_end, SIGNAL(dateChanged(const QDate&)), this, SLOT(dateChangedSlot(const QDate&)));
}

void ZNotes::ChangeFilter()
{
	QList<int> hideColumns;
	QStringList headers;

	hideColumns << 0;
	headers << tr("id") << tr("ФИО") << tr("Начало") << tr("Окончание")  << tr("Заметка");

	QString query = QString("SELECT n.id, fio.name, begin_dt, end_dt, note FROM public.notes2fio AS n \
INNER JOIN fio ON(n.fio = fio.id) WHERE begin_dt>='%1' AND end_dt<='%2'")
		.arg(beginDate.toString(DATE_FORMAT)).arg(endDate.toString(DATE_FORMAT));

	ui.m_tbl->setQuery(query, headers);

	ui.m_tbl->setReadOnly(false, false, false);

	ui.m_tbl->init(hideColumns);
}

void ZNotes::dateChangedSlot(const QDate& date)
{
	if (sender() == ui.date_begin)
		beginDate = date;
	else if (sender() == ui.date_end)
		endDate = date;
	else return;

	ChangeFilter();
}

void ZNotes::init(const QString& m_TblName)
{
	ui.m_tbl->setTable(m_TblName);
	ui.m_tbl->setCustomEditor(new ZNotesForm(this));

	ChangeFilter();

	ui.m_tbl->changeFilter(1);
}


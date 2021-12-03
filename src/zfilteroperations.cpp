#include <QMessageBox>
#include "zfilteroperations.h"

ZFilterOperations::ZFilterOperations(QWidget *parent)
	: QDialog(parent)
{
	setAttribute(Qt::WA_DeleteOnClose);

	m_tbl = new ZView(this);
	QGridLayout *pLayput = new QGridLayout(this);
	pLayput->addWidget(m_tbl);
	pLayput->setContentsMargins( 0, 0, 0, 0 );
	setLayout(pLayput); 
}

void ZFilterOperations::initDB(QSqlDatabase &m_DB, const QString &m_Filter)
{
	m_tbl->setDatabase(m_DB);
/*
  id serial NOT NULL,
  date text, -- Дата операции
  type integer, -- Тип: 0-Поступление/1-Выплата/2-Перемещение
  comment text, -- Комментарий
  ur_person integer, -- Юр.лицо
  partner integer, -- контрагент
  section integer, -- статья
  project integer, -- проект
  val double precision, -- Сумма
  account integer, -- Счет
  num integer, -- Номер операции
*/

	QString m_Query = QString("SELECT operations.date, operations.val, ur_persons.name, accounts.name, partners.name, projects.name, operations.comment FROM operations \
		INNER JOIN ur_persons ON (operations.ur_person = ur_persons.id)\
		INNER JOIN partners ON (operations.partner = partners.id)\
		INNER JOIN projects ON (operations.project = projects.id)\
		INNER JOIN accounts ON (operations.account = accounts.id)\
		WHERE operations.id IN (%1)").arg(m_Filter);

	QList<int> hideColumns;
	QStringList headers;

	headers << tr("Дата операции") << tr("Сумма")<< tr("Юр.лицо") << tr("Счёт")  << tr("Контрагент") << tr("Проект") << tr("Комментарий") ;

	m_tbl->setQuery(m_Query, headers);
		
	m_tbl->init(hideColumns, 0);
	//m_tbl->moveSection(3, 8);
}


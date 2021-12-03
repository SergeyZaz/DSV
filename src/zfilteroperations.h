#pragma once

#include <QDialog>
#include <QSqlDatabase>
#include "zview.h"

class ZFilterOperations : public QDialog
{
	ZView *m_tbl;
public:
    ZFilterOperations(QWidget *parent);
 
	void initDB(QSqlDatabase &m_DB, const QString &m_Query);
};



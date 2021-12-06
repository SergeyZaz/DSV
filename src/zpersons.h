#pragma once

#include "zmdichild.h"


class ZPersons : public ZMdiChild
{

public:
    ZPersons(QWidget* parent, Qt::WindowFlags flags = 0);
 
	void initDB(QSqlDatabase &m_DB, const QString &m_TblName);
};



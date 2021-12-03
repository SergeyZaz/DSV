#pragma once

#include "zmdichild.h"


class ZPersons : public ZMdiChild
{

public:
    ZPersons();
 
	void initDB(QSqlDatabase &m_DB, const QString &m_TblName);
};



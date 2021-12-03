#pragma once
#include "zviewgroups.h"

class ZGroups : public ZViewGroups
{
public:
	ZGroups();
 
	void initDB(QSqlDatabase &m_DB, const QString &m_TblName);
};



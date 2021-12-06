#pragma once
#include "zviewgroups.h"

class ZGroups : public ZViewGroups
{
public:
	ZGroups(QWidget* parent, Qt::WindowFlags flags = 0);
 
	void initDB(QSqlDatabase &m_DB, const QString &m_TblName);
};



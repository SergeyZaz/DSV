#pragma once

#include "zmdichild.h"


class ZSmens : public ZMdiChild
{

public:
	ZSmens(QWidget* parent, Qt::WindowFlags flags = 0);
 
	void initDB(QSqlDatabase &m_DB, const QString &m_TblName);
};



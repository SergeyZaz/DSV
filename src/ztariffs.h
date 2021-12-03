#pragma once

#include "zmdichild.h"


class ZTariffs : public ZMdiChild
{

public:
	ZTariffs();
 
	void initDB(QSqlDatabase &m_DB, const QString &m_TblName);
};



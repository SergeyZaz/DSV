#pragma once

#include "zmdichild.h"

class ZPayments : public ZMdiChild
{

public:
	ZPayments();

	void initDB(QSqlDatabase& m_DB, const QString& m_TblName);
};


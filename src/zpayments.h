#pragma once

#include "zmdichild.h"

class ZPayments : public ZMdiChild
{

public:
	ZPayments(QWidget* parent, Qt::WindowFlags flags = 0);

	void initDB(QSqlDatabase& m_DB, const QString& m_TblName);
};


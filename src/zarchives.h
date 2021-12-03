#pragma once

#include "zmdichild.h"

class ZArchives : public ZMdiChild
{

public:
	ZArchives();

	void initDB(QSqlDatabase& m_DB, const QString& m_TblName);
};


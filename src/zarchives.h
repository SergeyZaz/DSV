#pragma once

#include "zmdichild.h"

class ZArchives : public ZMdiChild
{

public:
	ZArchives(QWidget* parent, Qt::WindowFlags flags = 0);

	void initDB(QSqlDatabase& m_DB, const QString& m_TblName);
};


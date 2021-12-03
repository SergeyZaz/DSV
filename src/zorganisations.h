#pragma once
#include "zviewgroups.h"

class ZOrganisations : public ZViewGroups
{
public:
	ZOrganisations();

	void initDB(QSqlDatabase &m_DB, const QString &m_TblName);
};



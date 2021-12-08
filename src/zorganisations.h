#pragma once
#include "zviewgroups.h"

class ZOrganisations : public ZViewGroups
{
public:
	ZOrganisations(QWidget* parent, Qt::WindowFlags flags = 0);

	void init(const QString &m_TblName);
};



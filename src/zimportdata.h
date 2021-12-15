#pragma once

#include "zmdichild.h"


class ZImportData : public ZMdiChild
{

public:
	ZImportData(QWidget* parent, Qt::WindowFlags flags = 0);
 
	void init(const QString &m_TblName);
};



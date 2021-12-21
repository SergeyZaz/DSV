#pragma once

#include "zmdichild.h"


class ZNotes : public ZMdiChild
{

public:
	ZNotes(QWidget* parent, Qt::WindowFlags flags = 0);
 
	void init(const QString &m_TblName);
};



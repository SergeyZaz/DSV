#pragma once

#include "zmdichild.h"


class ZPersons : public ZMdiChild
{
	void execCustomAction(const QString&);

public:
    ZPersons(QWidget* parent, Qt::WindowFlags flags = 0);
 
	void init(const QString &m_TblName);
};



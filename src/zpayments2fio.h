#pragma once

#include "zmdichild.h"

class ZPayments2fio : public ZMdiChild
{

public:
	ZPayments2fio(QWidget* parent, Qt::WindowFlags flags = 0);

	void init(const QString& m_TblName);
};


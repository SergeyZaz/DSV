#pragma once

#include "zmdichild.h"


class ZImportData : public ZMdiChild
{
	Q_OBJECT

public:
	ZImportData(QWidget* parent, Qt::WindowFlags flags = 0);
 
	void init(const QString &m_TblName);

protected slots:
	void SelectionChanged(const QItemSelection&, const QItemSelection&);
};



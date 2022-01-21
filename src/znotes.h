#pragma once

#include "zmdichild.h"
#include "ui_znotes.h"


class ZNotes : public QWidget
{
	Q_OBJECT
		
	QDate beginDate, endDate;
	Ui::ZNotesDialog ui;

public:
	ZNotes(QWidget* parent, Qt::WindowFlags flags = 0);
 
	void init(const QString &m_TblName);

public slots:
	void ChangeFilter();
	void dateChangedSlot(const QDate&);
};



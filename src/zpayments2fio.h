#pragma once

#include "zmdichild.h"
#include "ui_zpayments2fio.h"

class ZPayments2fio : public QWidget
{
	Q_OBJECT

	int filterOrganisationId;
	int filterPaymentId;
	QDate filterDate;
	Ui::ZPayments2fioDialog ui;

public:
	ZPayments2fio(QWidget* parent, Qt::WindowFlags flags = 0);
	void init(const QString& m_TblName);

public slots:
	void UpdateSumma(int v = -1);
	void ChangeFilter();
	void dateChangedSlot(const QDate&);
	void ImportSlot();
	void SelectionChanged(const QItemSelection&, const QItemSelection&);
	QSize	sizeHint() const { return QSize(1000, 600); }
};


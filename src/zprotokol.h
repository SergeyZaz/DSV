#pragma once
#include <QDialog>
#include "ui_zprotokol.h"

class ZProtokol : public QDialog
{
	Q_OBJECT

	struct tariff_history
	{
		QDate date;
		double val;
	};
	struct tariff
	{
		int id, mode, type;
		double bonus;
		QString name;
		QList< tariff_history > l_history;
	};
	QList<tariff> l_tariffs;

	double getTariffValue(const QDate& date, int id, int num, QString& txt, double& bonus);
	void loadItemsTo(QComboBox* cbo, const QString& tableName);
	void loadTariffs();
	QSize	sizeHint() const { return QSize(1200, 600); }
	double getSumma(QTreeWidgetItem* pItemRoot, int col);

public:
	ZProtokol(QWidget* parent, Qt::WindowFlags flags = 0);
	~ZProtokol();

public slots:
	void buildProtokol();
	void saveProtokol();

private:
	Ui::ZProtokol ui;
};


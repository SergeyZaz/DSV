#pragma once
#include <QItemDelegate>
#include <QListWidget>
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
	QSize	sizeHint() const { return QSize(1400, 600); }
	double getSumma(QTreeWidgetItem* pItemRoot, int col);

public:
	ZProtokol(QWidget* parent, Qt::WindowFlags flags = 0);
	~ZProtokol();
	int getTextForPayment(int id, int col, QString& text, QVariantList &vList, double &summa);
	void updateSumm();

	Ui::ZProtokol ui;

public slots:
	void buildProtokol();
	void saveProtokol();
};


class ZTreeDataDelegate : public QItemDelegate
{
	Q_OBJECT

	ZProtokol* pEditor;
	mutable QWidget* w;
	mutable QListWidget* listWidget;
	mutable int column;
	mutable int fio_id;
	
	int openEditor(int id);

public:
	ZTreeDataDelegate(ZProtokol *Editor, QObject* parent = 0);

	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
		const QModelIndex& index) const;

	void setEditorData(QWidget* editor, const QModelIndex& index) const;

	void setModelData(QWidget* editor, QAbstractItemModel* model,
		const QModelIndex& index) const;

	void updateEditorGeometry(QWidget* editor,
		const QStyleOptionViewItem& option, const QModelIndex& index) const;

public slots:
	void add_clicked();
	void edit_clicked();
	void del_clicked();
};
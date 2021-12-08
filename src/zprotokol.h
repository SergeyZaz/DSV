#pragma once
#include <QItemDelegate>
#include <QTextEdit>
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
	QString getTextForPayment(int id, int col);

public slots:
	void buildProtokol();
	void saveProtokol();

private:
	Ui::ZProtokol ui;
};


class ZTreeDataDelegate : public QItemDelegate
{
	Q_OBJECT

	ZProtokol* pEditor;
	mutable QTextEdit* txtEdit;
	mutable int column;
	mutable int id;

public:
	ZTreeDataDelegate(ZProtokol *Editor, QObject* parent = 0);

	void commitUserData() { if(txtEdit) commitData(txtEdit); }

	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
		const QModelIndex& index) const;

	void setEditorData(QWidget* editor, const QModelIndex& index) const;

	void setModelData(QWidget* editor, QAbstractItemModel* model,
		const QModelIndex& index) const;

	void updateEditorGeometry(QWidget* editor,
		const QStyleOptionViewItem& option, const QModelIndex& index) const;

public slots:
	void b_clicked();
};
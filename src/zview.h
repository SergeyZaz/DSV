#pragma once

#include <QWidget>
#include <QSqlRelationalTableModel>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlQueryModel>
#include <QSortFilterProxyModel>
#include <QItemSelectionModel>
#include <QPointer>
#include "ui_zview.h"
#include "stdio.h"
#include "zeditbaseform.h"

#define SORT_ROLE	Qt::UserRole+99
#define DATE_FORMAT "yyyy-MM-dd"

class ZTableModel :  public QSqlRelationalTableModel
{
public:
	ZTableModel(QObject * parent = 0);
	~ZTableModel();
	QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	Qt::ItemFlags flags ( const QModelIndex & index ) const;
	int m_HighlightColumn;
	QList<int> *pHighlightItems;
	QMap< int , QMap<int, QString>* > relMaps;
	bool fEdit;
};

class ZQueryModel : public QSqlQueryModel
{
public:
	ZQueryModel(QObject* parent = 0);
	~ZQueryModel();
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	int m_HighlightColumn;
	QMap<int, QColor> mHighlightItems;
};

class ZSortFilterProxyModel : public QSortFilterProxyModel
{
	bool lessThan ( const QModelIndex & left, const QModelIndex & right ) const;
public:
	ZSortFilterProxyModel(){};
	~ZSortFilterProxyModel(){};
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
};

class ZView : public QWidget
{
	Q_OBJECT

	ZSortFilterProxyModel	sortModel;
	QSqlQueryModel			*model;
	QPointer<ZEditAbstractForm>		pEditForm;
	QString					mTable;
	int						m_Id;
	bool					fResizeColumnsToContents;
	bool					fResizeRowsToContents;

public:
	ZView(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	~ZView();
	void init();
	void setColorHighligthIfColumnContain(int col, QList<int> *plist, const QColor& c);
	void setColorHighligthIfColumnContain(int col, int val, const QColor& c);
	int setTable(const QString& tbl, QStringList& headers, QList<int>& removeColumns);
	void setTable(const QString& tbl) { mTable = tbl; }
	QString &getTable() { return mTable; }
	int setQuery(const QString &query, QStringList &headers, bool fRemoveOldModel = true);
	int init(QList<int> &hideColumns, int sortCol = 1, Qt::SortOrder s = Qt::AscendingOrder);
	void setRelation(int column, const QString &tbl, const QString &attId, const QString &attValue);
	void setRelation(int column, QMap<int, QString> *relMap);
	void updateAndShow(bool fMaximized = false);
	void moveSection(int from, int to);
	void setFilter(const QString & filter);
	void setCustomEditor(ZEditAbstractForm *pD);
	void setReadOnly(bool fEdit, bool fAdd, bool fDel);
	void setVisiblePrint(bool fTrue);
	QItemSelectionModel *selectionModel();
	QSqlQueryModel *getModel() { return model;}
	ZSortFilterProxyModel *getSortModel() { return &sortModel;}
	QTableView *getTblView() { return ui.tbl;}
	//bool loadView(int sortCol);
	QSize	sizeHint() const { return QSize(800, 600); }
	void setResizeColumnsToContents(bool f) { fResizeColumnsToContents = f; }
	void setResizeRowsToContents(bool f) { fResizeRowsToContents = f; }
private:
	Ui::ZViewClass ui;
	bool eventFilter(QObject *obj, QEvent *event);
	int openEditor(int id=ADD_UNIC_CODE);
	void update();
public slots:
	void add();
	void del();
	void edit();
	void changeFilter(const QString &);
	void changeFilter(int);
	void clickedTbl(const QModelIndex &);
	void doubleClickedTbl(const QModelIndex &);
	void selectionChanged(const QModelIndex&, const QModelIndex&);
	void reload();
	void applyEditor();
	void errorQuerySlot(const QDateTime&, long, const QString&);
signals:	
	void errorQuery(const QDateTime &, long , const QString &);
	void setCurrentElem( QEvent::Type, int );
	void needUpdate();
	void needUpdateVal(int);
};

double QString2Double(QString txt);
void loadItemsToComboBox(QComboBox* cbo, const QString& tableName);
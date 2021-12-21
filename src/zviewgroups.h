#pragma once
#include <QWidget>
#include <QSqlQueryModel>
#include <QSortFilterProxyModel>
#include "ui_zviewgroups.h"


class ZFioModel : public QSqlQueryModel
{
	QList<int> l_Ids;
public:
	ZFioModel() {};
	~ZFioModel() {};
	void setHighlightItems(const QList<int>& l) { l_Ids = l; }
	QVariant data(const QModelIndex& index, int role) const;
};

	
class ZViewGroups : public QWidget
{
	Q_OBJECT

	ZFioModel* modelFIO;
	QSqlQueryModel* model;
	QSortFilterProxyModel sortModelFIO, sortModel;
	enum OPERATION { INSERT_OPERATION, DELETE_OPERATION };
	QString linkTableName;
	int currentId;
	
	void updateGroups(QTableView* tbl, OPERATION operation);
	void Update();

public:
	Ui::ZGroups ui;

	ZViewGroups(QWidget* parent = NULL, Qt::WindowFlags flags = 0);
	~ZViewGroups() {};

	void setWindowTitleAndIcon(const QString& title, const QIcon& icon)
	{
		setWindowTitle(title);
		setWindowIcon(icon);
	}
	void setup();
	void setLinkTableName(const QString& tbl);

public slots:
	void changeFilterFIO(const QString&);
	void toLeftSlot();
	void toRightSlot();
	void setCurrentElem(QEvent::Type, int); 
	void moveElemSlot(const QModelIndex& index);

};



#include "zmessager.h"
#include "zviewgroups.h"

#define HIGHLIGHTCOLOR Qt::cyan

ZViewGroups::ZViewGroups(QWidget* parent, Qt::WindowFlags flags)//: QDialog(parent, flags)
{
	ui.setupUi(this);
	
	modelFIO = NULL;
	currentId = -1;
	model = NULL;

	connect(ui.txtFilter_3, SIGNAL(textChanged(const QString&)), this, SLOT(changeFilterFIO(const QString&)));
	connect(ui.txtFilter_2, SIGNAL(textChanged(const QString&)), this, SLOT(changeFilterFIO(const QString&)));
	connect(ui.toLeftToolButton, SIGNAL(clicked()), this, SLOT(toLeftSlot()));
	connect(ui.toRightToolButton, SIGNAL(clicked()), this, SLOT(toRightSlot()));
	connect(ui.m_tbl, SIGNAL(setCurrentElem(QEvent::Type, int)), this, SLOT(setCurrentElem(QEvent::Type, int)));
	connect(ui.tbl_2, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(moveElemSlot(const QModelIndex&)));
	connect(ui.tbl_3, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(moveElemSlot(const QModelIndex&)));
}

void ZViewGroups::setup()
{
	modelFIO = new ZFioModel();

	modelFIO->setQuery("SELECT id,name FROM public.fio");
	if (modelFIO->lastError().isValid())
	{
		QApplication::restoreOverrideCursor();
		ZMessager::Instance().Message(_CriticalError, modelFIO->lastError().text(), "Ошибка");
		return;
	}

	sortModelFIO.setSourceModel(modelFIO);
	sortModelFIO.setFilterKeyColumn(1);

	modelFIO->setHeaderData(1, Qt::Horizontal, "ФИО");

	ui.tbl_3->setModel(&sortModelFIO);
	ui.tbl_3->setColumnHidden(0, true);
	ui.tbl_3->verticalHeader()->hide();
	ui.tbl_3->horizontalHeader()->setSortIndicator(1, Qt::AscendingOrder);
	//ui.tbl_3->horizontalHeader()->setStretchLastSection(true);

	Update();
}

void ZViewGroups::changeFilterFIO(const QString& text)
{
	if (!modelFIO)
		return;

	QSortFilterProxyModel* pSortModel = &sortModelFIO;
	if (sender() == ui.txtFilter_2)
		pSortModel = &sortModel;

	QRegExp regExp(text, Qt::CaseInsensitive);
	pSortModel->setFilterRegExp(regExp);
}

void ZViewGroups::toLeftSlot()
{
	updateGroups(ui.tbl_3, ZViewGroups::INSERT_OPERATION);
}

void ZViewGroups::toRightSlot()
{
	updateGroups(ui.tbl_2, ZViewGroups::DELETE_OPERATION);
}

void ZViewGroups::moveElemSlot(const QModelIndex& index)
{
	if(sender() == ui.tbl_3)
		updateGroups(ui.tbl_3, ZViewGroups::INSERT_OPERATION);
	else
		updateGroups(ui.tbl_2, ZViewGroups::DELETE_OPERATION);
}

void ZViewGroups::setLinkTableName(const QString& tbl)
{ 
	linkTableName = tbl; 
	Update();
}

void  ZViewGroups::Update()
{
	if (!modelFIO)
		return;

	QSqlQuery query;
	if (query.exec(QString("SELECT distinct(value) FROM %1").arg(linkTableName)))
	{
		QList<int> ids;
		while (query.next())
			ids << query.value(0).toInt();
		modelFIO->setHighlightItems(ids);
	}

	if (model)
		delete model;

	model = new QSqlQueryModel();

	model->setQuery(QString("SELECT value,name FROM %2 INNER JOIN fio ON (value = fio.id) WHERE key=%1").arg(currentId).arg(linkTableName));

	sortModel.setSourceModel(model);
	sortModel.setFilterKeyColumn(1);

	model->setHeaderData(1, Qt::Horizontal, "ФИО");

	ui.tbl_2->setModel(&sortModel);
	ui.tbl_2->setColumnHidden(0, true);
	ui.tbl_2->verticalHeader()->hide();
	ui.tbl_2->horizontalHeader()->setSortIndicator(1, Qt::AscendingOrder);
	//ui.tbl_2->horizontalHeader()->setStretchLastSection(true);

	ui.tbl_3->viewport()->update();
}

void ZViewGroups::setCurrentElem(QEvent::Type, int id)
{
	currentId = id;

	QSqlQuery query;
	int rc = query.exec(QString("SELECT name FROM %1 WHERE id=%2").arg(ui.m_tbl->getTable()).arg(currentId));
	if (rc && query.next())
		ui.group->setTitle(query.value(0).toString());
	else
		ui.group->setTitle("");

	Update();
}

void ZViewGroups::updateGroups(QTableView* tbl, OPERATION operation)
{
	if (!tbl)
		return;

	QString stringQuery = "";	
	QModelIndexList listIndxs;
	int id;
	QModelIndex indx;

	if (currentId < 0)
	{
		ZMessager::Instance().Message(_Error, "Выберите элемент из списка и повторите попытку", "Внимание");
		return;
	}

	QSortFilterProxyModel* pSortModel = NULL;

	if (operation == INSERT_OPERATION)
	{
		stringQuery = QString("INSERT INTO %1(key, value) VALUES ").arg(linkTableName);
		pSortModel = &sortModelFIO;
	}

	if (operation == DELETE_OPERATION)
	{
		stringQuery = QString("DELETE FROM %1 WHERE key=%2 AND value IN (").arg(linkTableName).arg(currentId);
		pSortModel = &sortModel;
	}

	if (!pSortModel)
		return;

	listIndxs = tbl->selectionModel()->selectedRows();
	if (listIndxs.size() == 0)
	{
		ZMessager::Instance().Message(_Error, "Выберите элементы из списка и повторите попытку", "Внимание");
		return;
	}

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	foreach(indx, listIndxs)
	{
		id = pSortModel->data(pSortModel->index(indx.row(), 0)).toInt();
		if (operation == INSERT_OPERATION)
			stringQuery += QString("(%1, %2),").arg(currentId).arg(id);
		if (operation == DELETE_OPERATION)
			stringQuery += QString("%1,").arg(id);
	}

	stringQuery.chop(1);

	if (operation == DELETE_OPERATION)
		stringQuery.append(")");

	QSqlQuery query;
	int rc = query.exec(stringQuery);

	QApplication::restoreOverrideCursor();

	if(!rc)
		ZMessager::Instance().Message(_Error, query.lastError().text(), "Ошибка");

	Update();
}

/////////////////////////////////////////////////////////////////////////////////////
QVariant ZFioModel::data(const QModelIndex& index, int role) const
{
	int col = index.column();
	int row = index.row();

	if (role == Qt::BackgroundColorRole)
	{
		QModelIndex t_indx = this->index(row, 0);
		QVariant v = QSqlQueryModel::data(t_indx, Qt::DisplayRole);
		if (!l_Ids.contains(v.toInt()))
			return QColor(HIGHLIGHTCOLOR);
	}
	return QSqlQueryModel::data(index, role);
}
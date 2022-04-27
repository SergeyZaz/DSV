#include <QDateTime>
#include <QDateTimeEdit>
#include <QHelpEvent>
#include <QMessageBox>
#include <QPainter>
#include <QHeaderView>
#include <QCompleter>
#include "zview.h"
#include "ztoolwidget.h"
#include "zmessager.h"

#define HIGHLIGHTCOLOR Qt::cyan
#define PLUS_COLOR	QColor(85, 255, 127)
#define MINUS_COLOR	QColor(255, 170, 127)


QPixmap *ZToolWidget::pixmap = NULL;
ZToolWidget *ZToolWidget::pZToolWidget = NULL;

ZView::ZView(QWidget *parent, Qt::WindowFlags flags)
	: QWidget(parent, flags)
{
	init();
}
	
void ZView::init()
{
	fResizeColumnsToContents = true;
	fResizeRowsToContents = false;
	m_Id = -1;
	model = NULL;
	ui.setupUi(this);
	//setAttribute(Qt::WA_DeleteOnClose);

	connect(ui.txtFilter, SIGNAL(textChanged(const QString &)), this, SLOT(changeFilter(const QString &)));
	connect(ui.cboFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(changeFilter(int)));
	connect(ui.cmdAdd, SIGNAL(clicked()), this, SLOT(add()));
	connect(ui.cmdDel, SIGNAL(clicked()), this, SLOT(del()));
	connect(ui.cmdEdit, SIGNAL(clicked()), this, SLOT(edit()));
	connect(ui.tbl, SIGNAL(clicked ( const QModelIndex &)), this, SLOT(clickedTbl ( const QModelIndex &)));
	connect(ui.tbl, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(doubleClickedTbl(const QModelIndex&)));
	connect(ui.cmdReload, SIGNAL(clicked()), this, SLOT(reload()));
	
	
	sortModel.setDynamicSortFilter(true); 
	sortModel.setFilterKeyColumn(1); 

	ui.tbl->horizontalHeader()->setSectionsMovable(true);
	//ui.tbl->verticalHeader()->hide();
	ui.tbl->installEventFilter(this);

	ui.cmdPrint->setVisible(false);
	
	bool rc;
	pEditForm = new ZEditBaseForm(this);
	rc = connect(pEditForm, SIGNAL(accepted()), this, SLOT(applyEditor()));
	rc = connect(pEditForm, SIGNAL(errorQuery(const QDateTime&, long, const QString&)), this, SIGNAL(errorQuery(const QDateTime&, long, const QString&)));

	rc = connect(this, SIGNAL(errorQuery(const QDateTime&, long, const QString&)), this, SLOT(errorQuerySlot(const QDateTime&, long, const QString&)));
}

ZView::~ZView()
{
	if(pEditForm)
	{
		delete pEditForm;
		pEditForm = NULL;
	}
	if(model)
		delete model;
}
	
void ZView::errorQuerySlot(const QDateTime&, long, const QString& text)
{
	ZMessager::Instance().Message(_CriticalError, text);
}

void ZView::setColorHighligthIfColumnContain(int col, QList<int> *plist, const QColor &c)
{
	ZTableModel *pModel = dynamic_cast<ZTableModel*>(model);
	if(!pModel || !plist)
		return;
	pModel->pHighlightItems = plist;
	pModel->m_HighlightColumn = col;
}

void ZView::setColorHighligthIfColumnContain(int col, int val, const QColor& c)
{
	ZQueryModel* pModel = dynamic_cast<ZQueryModel*>(model);
	if (!pModel)
		return;
	pModel->mHighlightItems.insert(val, c);
	pModel->m_HighlightColumn = col;
}

void ZView::setRelation(int column, const QString &tbl, const QString &attId, const QString &attValue)
{
	ZTableModel *pModel = dynamic_cast<ZTableModel*>(model);
	if(pModel) 
		pModel->setRelation(column, QSqlRelation(tbl, attId, attValue));
}
	
void ZView::setRelation(int column, QMap<int, QString> *relMap)
{
	ZTableModel *pModel = dynamic_cast<ZTableModel*>(model);
	if(pModel) 
		pModel->relMaps.insert(column, relMap);
}

int ZView::setTable(const QString &tbl, QStringList &headers, QList<int> &removeColumns)
{
	if(pEditForm)
	{
		if(windowTitle() != "ZView")
		{
			QString text = QString("%1 \"%2\"").arg(pEditForm->windowTitle()).arg(windowTitle());
			pEditForm->setWindowTitle(text);
		}
	}

	mTable = tbl;

	ZTableModel *pModel = new ZTableModel(this);
	pModel->setTable(tbl);
	pModel->setEditStrategy(QSqlTableModel::OnFieldChange);

	sortModel.setSourceModel(pModel); 
	model = pModel;

	int indx;

	for(indx=0; indx<headers.size(); indx++)
	{
		QString text = headers.at(indx);
		model->setHeaderData(indx, Qt::Horizontal, text);
		ui.cboFilter->addItem(text, indx);
	}

	qSort(removeColumns.begin(), removeColumns.end(), qGreater<int>());
	foreach(indx, removeColumns)
	{
		model->removeColumn(indx);
	}
	return 1;
}

int ZView::setQuery(const QString &query, QStringList &headers, bool fRemoveOldModel)
{
	if(model && fRemoveOldModel)
	{
		delete model;
		model = NULL;
	}

	ui.cmdAdd->setVisible(false);
	ui.cmdDel->setVisible(false);
	ui.cmdEdit->setVisible(false);
	//ui.cmdReload->setVisible(false);

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	if(!model)
		model = new ZQueryModel();

	model->setQuery(query);
	if (model->lastError().isValid())
	{
		QApplication::restoreOverrideCursor();
		emit errorQuery(QDateTime::currentDateTime(), model->lastError().number(), model->lastError().text());
		return 0;
	}

	sortModel.setSourceModel(model); 

	int indx;
	
	ui.cboFilter->clear();

	for(indx=0; indx<headers.size(); indx++)
	{
		QString text = headers.at(indx);
		model->setHeaderData(indx, Qt::Horizontal, text);
		ui.cboFilter->addItem(text, indx);
	}
	
	QApplication::restoreOverrideCursor();
	return 1;
}

int ZView::init(QList<int> &hideColumns, int sortCol, Qt::SortOrder s)
{
	if(!model)
		return 0;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	ZTableModel *pModel = dynamic_cast<ZTableModel*>(model);
	if(pModel)
	{
		pModel->select();
//		sortModel.setSourceModel(model); 

		if (model->lastError().isValid())
		{
			QApplication::restoreOverrideCursor();
			emit errorQuery(QDateTime::currentDateTime(), model->lastError().number(), model->lastError().text());
			return 0;
		}
	}

	//ui.tbl->horizontalHeader()->setSectionsMovable(true);
	ui.tbl->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);//ResizeToContents);//QHeaderView::Interactive);
	ui.tbl->setModel(&sortModel);
	bool rc = connect(ui.tbl->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(selectionChanged(const QModelIndex&, const QModelIndex&)));

	int indx;
	qSort(hideColumns.begin(), hideColumns.end(), qGreater<int>());
	foreach(indx, hideColumns)
	{
		ui.tbl->setColumnHidden(indx, true);
		ui.cboFilter->removeItem(indx);
	}

	ui.tbl->horizontalHeader()->setSortIndicator(sortCol, s); 
	
	ui.tbl->verticalHeader()->setDefaultSectionSize(30);

	if (fResizeColumnsToContents)
		ui.tbl->resizeColumnsToContents();
	if (fResizeRowsToContents)
		ui.tbl->resizeRowsToContents();

	QApplication::restoreOverrideCursor();
	return 1;
}

void ZView::moveSection(int from, int to)
{
	ui.tbl->horizontalHeader()->moveSection(from, to);
}
	
void ZView::update()
{
	if(!model)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	
	ZTableModel *pModel = dynamic_cast<ZTableModel*>(model);
	if(pModel)
	{
		pModel->select();

		if (model->lastError().isValid())
		{
			QApplication::restoreOverrideCursor();
			emit errorQuery(QDateTime::currentDateTime(), model->lastError().number(), model->lastError().text());
			return;
		}
	}
	else
	{
		model->setQuery(model->query().lastQuery());
	}

	emit needUpdate();
	//emit needUpdateVal(-1);

	if (model && m_Id != -1)
	{
		QModelIndex index;
		int i, n = model->rowCount();
		for (i = 0; i < n; i++)
		{
			index = model->index(i, 0);
			if (model->data(index).toInt() == m_Id)
			{
				index = model->index(i, 1);
				break;
			}
		}
		if (index.isValid() && i != n)
		{
			index = sortModel.mapFromSource(index);
			ui.tbl->setCurrentIndex(index);
			ui.tbl->scrollTo(index, QAbstractItemView::PositionAtCenter);
		}
	}

	if (fResizeColumnsToContents)
		ui.tbl->resizeColumnsToContents();
	if (fResizeRowsToContents)
		ui.tbl->resizeRowsToContents();

	QApplication::restoreOverrideCursor();
}

void ZView::updateAndShow(bool fMaximized)
{
	update();

	//делаем окно активным
	if (isMinimized())
		setWindowState(windowState() &  ~Qt::WindowMinimized | Qt::WindowActive);
	else
		show();

	activateWindow();

	raise();
	
	if(fMaximized)
		showMaximized();
	else
		show();
}

void ZView::reload()
{
	update();
}

void ZView::add()
{
	//ZTableModel *pModel = dynamic_cast<ZTableModel*>(model);
	//if(pModel) 
	//	pModel->insertRows(0, 1); 
	openEditor();
}

void ZView::edit()
{
	if(!model)
		return;
	QModelIndexList listIndxs = ui.tbl->selectionModel()->selectedRows();
	if(listIndxs.size()==0)
	{
		QMessageBox::warning( this, tr("Внимание"), tr("Выберите элемент из списка и повторите попытку"));
		return;
	}
	QModelIndex indx = sortModel.mapToSource(listIndxs.first());
	int id = model->data(model->index(indx.row(), 0)).toInt();

	openEditor(id);
}

int ZView::openEditor(int id)
{
	if(!model)
		return 0;

	m_Id = id;
	printf("edit id = %d\n", id);

	if(!pEditForm || (id == 0  && mTable != "users")|| id == -1)
		return 0;

	if(!pEditForm->init(mTable, id))
		return 0;
	
	pEditForm->show();
	return 1;
}

void ZView::applyEditor()
{
	update();
}
	
void ZView::setCustomEditor(ZEditAbstractForm *pD)
{
	if(pEditForm)
		delete pEditForm;
	pEditForm = pD;
	bool rc;
	rc = connect(pEditForm, SIGNAL(needUpdateVal(int)), this, SIGNAL(needUpdateVal(int)));
	rc = connect(pEditForm, SIGNAL(accepted()), this, SLOT(applyEditor()));
	rc = connect(pD, SIGNAL(errorQuery(const QDateTime &, long , const QString &)), this, SIGNAL(errorQuery(const QDateTime &, long , const QString &)));
}

void ZView::del()
{
	if(!model)
		return;

	QModelIndexList listIndxs = ui.tbl->selectionModel()->selectedRows();
	QList<int> ids;
	int tId;
	foreach(QModelIndex index, listIndxs)
	{
		QModelIndex indx = sortModel.mapToSource(index);
		tId = model->data(model->index(indx.row(), 0)).toInt(); // id должен быть 0-м столюцом!!!
		if(tId!=0 && !ids.contains(tId))
			ids.push_back(tId);
	}
	
	if(!ids.size())
		return;
	if ( QMessageBox::question( this, tr("Внимание"), tr("Вы действительно хотите удалить выделенные элементы?"), tr("Да"), tr("Нет"), QString::null, 0, 1 ) != 0)
		return;

	QSqlQuery m_Query;
	QString s_Query = tr("DELETE FROM %1 WHERE id IN (").arg(mTable);

	for(int i=0;i<ids.size();i++)
	{
		printf("remove id = %d\n", ids[i]);

		if(i!=0)
			s_Query += ",";

		s_Query += QString::number(ids[i]);
	}

	s_Query += ")";
			
	if(!m_Query.exec(s_Query))
	{
		emit errorQuery(QDateTime::currentDateTime(), m_Query.lastError().number(), m_Query.lastError().text());
		return;
	}

	emit needUpdate();

	update();
}

void ZView::changeFilter(int indx)
{
	if(!model)
		return;

	ui.cboFilter->blockSignals(true);
	ui.cboFilter->setCurrentIndex(indx);
	ui.cboFilter->blockSignals(false);

	sortModel.setFilterKeyColumn(ui.cboFilter->itemData(indx).toInt());

	QRegExp	regExp(ui.txtFilter->text(), Qt::CaseInsensitive, QRegExp::FixedString);
	sortModel.setFilterRegExp(regExp);
	
	emit needUpdateVal(-1);
}

void ZView::changeFilter(const QString &text)
{
	if(!model)
		return;
	
	QRegExp	regExp(text, Qt::CaseInsensitive, QRegExp::FixedString);
	sortModel.setFilterRegExp(regExp);

	emit needUpdateVal(-1);
}

bool ZView::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		QPixmap pixmap;
		QHelpEvent *pHelpEvent = dynamic_cast<QHelpEvent*>(event);
		bool rc = false;
		if(pHelpEvent)
			rc = ZToolWidget::Show(pHelpEvent->globalPos());
		return rc;
	} 
	return QObject::eventFilter(obj, event);
}

void ZView::clickedTbl(const QModelIndex &index)
{
	if(!model)
		return;
	QModelIndex indx = sortModel.mapToSource(index);
	
	m_Id = model->data(model->index(indx.row(), 0)).toInt();

	ui.cmdDel->setEnabled(m_Id > 0); // удалить нельзя
	ui.cmdEdit->setEnabled(m_Id > 0); // редактировать нельзя

	emit setCurrentElem(QEvent::MouseButtonRelease, m_Id);
}

void ZView::selectionChanged(const QModelIndex& current, const QModelIndex&)
{
	if (!model)
		return;
	QModelIndex indx = sortModel.mapToSource(current);
	int id = model->data(model->index(indx.row(), 0)).toInt();

	ui.cmdDel->setEnabled(id > 0); // удалить нельзя
	ui.cmdEdit->setEnabled(id > 0); // редактировать нельзя

	emit setCurrentElem(QEvent::ActivationChange, id);
}

void ZView::doubleClickedTbl(const QModelIndex &index)
{
	if(!model)
		return;
	QModelIndex indx = sortModel.mapToSource(index);
	int id = model->data(model->index(indx.row(), 0)).toInt();

	emit setCurrentElem(QEvent::MouseButtonDblClick, id);

	openEditor(id);
}

	
void ZView::setReadOnly(bool fEdit, bool fAdd, bool fDel)
{
	ui.cmdAdd->setVisible(!fAdd);
	ui.cmdDel->setVisible(!fDel);
	ui.cmdEdit->setVisible(!fEdit);
	ZTableModel *pModel = dynamic_cast<ZTableModel*>(model);
	if(pModel) 
		pModel->fEdit = !fEdit;
}
	
QItemSelectionModel *ZView::selectionModel()
{
	return ui.tbl->selectionModel();
}

void ZView::setFilter(const QString & filter)
{ 
	ZTableModel *pModel = dynamic_cast<ZTableModel*>(model);
	if(pModel) 
		pModel->setFilter(filter);
}

	
void ZView::setVisiblePrint(bool fTrue)
{
	ui.cmdPrint->setVisible(fTrue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
ZTableModel::ZTableModel(QObject *parent): QSqlRelationalTableModel(parent)
{
	m_HighlightColumn = -1;
	pHighlightItems = NULL;
	fEdit = false;
}
	
ZTableModel::~ZTableModel()
{
	QMapIterator<int , QMap<int, QString>* > iT(relMaps);
	while (iT.hasNext()) 
	{
		iT.next();
		delete iT.value();
	}	
	relMaps.clear();
}

QVariant ZTableModel::data(const QModelIndex & index, int role) const
{
	QVariant v;
	int col = index.column();
	int row = index.row();

	if(pHighlightItems && m_HighlightColumn!=-1 && role==Qt::BackgroundColorRole)
	{
		QModelIndex t_indx = this->index(row, 0);
		v = QSqlTableModel::data(t_indx, Qt::DisplayRole);
		if(pHighlightItems->contains(v.toInt()))
			return QColor(HIGHLIGHTCOLOR);
	}
/*
	if(role==Qt::BackgroundColorRole)
	{
		v = QSqlTableModel::data(index, Qt::DisplayRole);
		if(v.type() == QVariant::Double)
		{
			switch(QSqlTableModel::data(this->index(row, 2), Qt::DisplayRole).toInt())
			{
			case 1://Тип: 0-Поступление/1-Выплата/2-Перемещение
				return MINUS_COLOR;
			case 0:
				if(v.toDouble() < 0)
					return MINUS_COLOR;
				return PLUS_COLOR;
			default:
				return QColor(Qt::gray);
			}
		}
	}
*/
	if(role==Qt::TextAlignmentRole)
	{
		v = QSqlTableModel::data(index, Qt::DisplayRole);
		if(v.type() == QVariant::Double)
		{
			return Qt::AlignRight;
		}
	}

	v = QSqlTableModel::data(index, role);
	if (v.type() == QVariant::Date)
		return v.toDate().toString(DATE_FORMAT);

	if(v.type() == QVariant::Double)
#ifndef MONEY_FORMAT
		return QString::number(v.toDouble(), 'f', 2);
#else
		return QString("%L1").arg(v.toDouble(), 0, 'f', 2);
#endif

	switch(role)
	{
	case Qt::DisplayRole:
		{
			const QMap<int, QString> *pRelMap = relMaps.value(col, NULL);
			if(pRelMap)
				return pRelMap->value(v.toInt());
		}
		break;
	default:
		break;
	}

	return v;
}

Qt::ItemFlags ZTableModel::flags(const QModelIndex &) const
{
	Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	if(fEdit)
		flags |= Qt::ItemIsEditable;
	return flags;
}

/////////////////////////////////////////////////////////////////////////////////////

bool ZSortFilterProxyModel::lessThan(const QModelIndex &left,
                                       const QModelIndex &right) const
 {
	 bool ok;
	 QString leftData = sourceModel()->data(left).toString();
     QString rightData = sourceModel()->data(right).toString();
	 leftData = leftData.replace(QChar::Nbsp, "");
	 rightData = rightData.replace(QChar::Nbsp, "");
	 double d = leftData.toDouble(&ok);
	 if ( ok )
	 {
		 return d < rightData.toDouble();
     }
	 return QSortFilterProxyModel::lessThan(left, right);
 }
 
QVariant ZSortFilterProxyModel::data( const QModelIndex & index, int role) const
{
	QVariant v = QSortFilterProxyModel::data(index, role);
	
	if(v.type() == QVariant::Double)
#ifndef MONEY_FORMAT
		return QString::number(v.toDouble(), 'f', 2);
#else
		return QString("%L1").arg(v.toDouble(), 0, 'f', 2);
#endif
	return v;
}

QVariant ZSortFilterProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	// переопределение сортировки нумерации строк
	if (role == Qt::DisplayRole) 
	{
		if (orientation == Qt::Vertical) 
		{
			return section + 1;
		}
	}
	return QSortFilterProxyModel::headerData(section, orientation, role);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////

ZQueryModel::ZQueryModel(QObject* parent): QSqlQueryModel(parent)
{
	m_HighlightColumn = -1;
}

ZQueryModel::~ZQueryModel()
{
	
}

QVariant ZQueryModel::data(const QModelIndex& index, int role) const
{
	if (m_HighlightColumn != -1 && role == Qt::BackgroundColorRole)
	{
		int col = index.column();
		int row = index.row();
		QModelIndex t_indx = this->index(row, m_HighlightColumn);
		QVariant v = data(t_indx, Qt::DisplayRole);
		QColor c = mHighlightItems.value(v.toInt(), Qt::transparent);
		if (c != Qt::transparent)
			return c;
	}

	QVariant v = QSqlQueryModel::data(index, role);
	if (v.type() == QVariant::Date)
		return v.toDate().toString(DATE_FORMAT);

	return QSqlQueryModel::data(index, role);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

double QString2Double(QString txt)
{
	bool ok;
	txt.replace(QChar::Nbsp, "");
	double v = txt.toDouble(&ok);
	if (!ok)
	{
		txt.replace(",", ".");
		v = txt.toDouble(&ok);
	}
	return v;
}

void loadItemsToComboBox(QComboBox* cbo, const QString& tableName)
{
	QSqlQuery query;
	cbo->clear();
	if(tableName == "fio")
		cbo->addItem("не задано", 0);

	auto result = query.exec(QString("SELECT id, name FROM %1 ORDER BY name").arg(tableName));
	if (result)
	{
		while (query.next())
		{
			cbo->addItem(query.value(1).toString(), query.value(0).toInt());
		}
	}
	cbo->setCurrentIndex(cbo->findText("не задано"));

	cbo->setEditable(true);
	QCompleter* completer = new QCompleter();
	completer->setModel(cbo->model());
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	cbo->setCompleter(completer);
}

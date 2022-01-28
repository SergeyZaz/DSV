#include <QMessageBox>
#include "zmessager.h"
#include "ztariffs.h"
#include "ztariffsform.h"
#include "ztariffhistory.h"

QStringList ZTariffs::l_Modes; 
QStringList& ZTariffs::getModes()
{
	if (l_Modes.size() == 0)
		l_Modes << "станок ..." << "станок начинается с ..." << "станок содержит ..." << "станок заканчивается ..."
		<< "модель шины ..." << "модель шины начинается с ..." << "модель шины содержит ..." << "модель шины заканчивается ..."
		<< "Rate ..." << "Rate начинается с ..." << "Rate содержит ..." << "Rate заканчивается ...";
	return l_Modes;
}

ZTariffs::ZTariffs(QWidget* parent, Qt::WindowFlags flags)// : QDialog(parent, flags)
{
	model = NULL;
	currentId = -1;
	ui.setupUi(this);
	connect(ui.m_tbl, SIGNAL(setCurrentElem(QEvent::Type, int)), this, SLOT(setCurrentElem(QEvent::Type, int)));
	connect(ui.tbl, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(doubleClickedTbl(const QModelIndex&)));
	connect(ui.cmdAdd, SIGNAL(clicked()), this, SLOT(add()));
	connect(ui.cmdDel, SIGNAL(clicked()), this, SLOT(del()));
	connect(ui.cmdEdit, SIGNAL(clicked()), this, SLOT(edit()));
}


void ZTariffs::init()
{
	QList<int> hideColumns;
	QStringList headers;
	QList<int> cRem;
	
	hideColumns << 0;
	headers << tr("id") << tr("Название") << tr("Режим") << tr("Доплата") << tr("Вид") << tr("Комментарий") << tr("Приоритет");

	ui.m_tbl->setTable("tariff", headers, cRem);
	ui.m_tbl->setCustomEditor(new ZTariffsForm(this));

	//0 - станок ..., 1 - станок начинается с ..., 2 - станок содержит ..., 3 - станок заканчивается ...,
	//4 - модель шины ..., 5 - модель шины начинается с ..., 6 - модель шины содержит ..., 7 - модель шины заканчивается ...,
	//8 - Rate ..., 9 - Rate начинается с ..., 10 - Rate содержит ..., 11 - Rate заканчивается ...		
	QMap<int, QString> *pMap0 = new QMap<int, QString>;
	int i = 0;
	foreach(QString t, getModes())
		pMap0->insert(i++, t);
	ui.m_tbl->setRelation(2, pMap0);
	
	QMap<int, QString>* pMap1 = new QMap<int, QString>;
	pMap1->insert(0, "за смену");
	pMap1->insert(1, "за штуку");
	ui.m_tbl->setRelation(4, pMap1);

	ui.m_tbl->init(hideColumns, 6);

	ui.m_tbl->moveSection(6, 1);

	Update();
}

void ZTariffs::doubleClickedTbl(const QModelIndex& index)
{
	if (!model)
		return;
	QModelIndex indx = sortModel.mapToSource(index);
	int id = model->data(model->index(indx.row(), 0)).toInt();

	//emit setCurrentElem(QEvent::MouseButtonDblClick, id);

	openEditor(id);
}

void ZTariffs::setCurrentElem(QEvent::Type, int id)
{
	currentId = id;
	Update();
}

void ZTariffs::Update()
{
	if (currentId < 0)
		return;

	if (model)
		delete model;

	model = new QSqlQueryModel();

	model->setQuery(QString("SELECT id,dt,val FROM tariff_history WHERE tariff_id=%1").arg(currentId));

	sortModel.setSourceModel(model);
	sortModel.setFilterKeyColumn(1);

	model->setHeaderData(1, Qt::Horizontal, "Дата утверждения");
	model->setHeaderData(2, Qt::Horizontal, "Ставка");

	ui.tbl->setModel(&sortModel);
	ui.tbl->setColumnHidden(0, true);
	ui.tbl->verticalHeader()->hide();
	ui.tbl->horizontalHeader()->setSortIndicator(1, Qt::AscendingOrder);
	ui.tbl->setColumnWidth(1, 150);
	//ui.tbl->horizontalHeader()->setStretchLastSection(true);

	ui.tbl->viewport()->update();

}

void ZTariffs::add()
{
	openEditor();
}

void ZTariffs::del()
{
	if (!model)
		return;

	QModelIndexList listIndxs = ui.tbl->selectionModel()->selectedRows();
	QList<int> ids;
	int tId;
	foreach(QModelIndex index, listIndxs)
	{
		QModelIndex indx = sortModel.mapToSource(index);
		tId = model->data(model->index(indx.row(), 0)).toInt(); // id должен быть 0-м столюцом!!!
		if (tId != 0 && !ids.contains(tId))
			ids.push_back(tId);
	}

	if (!ids.size())
		return;
	if (QMessageBox::question(this, tr("Внимание"), tr("Вы действительно хотите удалить выделенные элементы?"), tr("Да"), tr("Нет"), QString::null, 0, 1) != 0)
		return;

	QSqlQuery m_Query;
	QString s_Query = tr("DELETE FROM tariff_history WHERE id IN (");

	for (int i = 0; i < ids.size(); i++)
	{
		if (i != 0)
			s_Query += ",";

		s_Query += QString::number(ids[i]);
	}

	s_Query += ")";

	if (!m_Query.exec(s_Query))
	{
		ZMessager::Instance().Message(_Error, m_Query.lastError().text());
		return;
	}

	Update();
}

void ZTariffs::edit()
{
	if (!model)
		return;
	QModelIndexList listIndxs = ui.tbl->selectionModel()->selectedRows();
	if (listIndxs.size() == 0)
	{
		ZMessager::Instance().Message(_Error, "Выберите элемент из списка и повторите попытку", "Внимание");
		return;
	}
	QModelIndex indx = sortModel.mapToSource(listIndxs.first());
	int id = model->data(model->index(indx.row(), 0)).toInt();

	openEditor(id);
}

int ZTariffs::openEditor(int id)
{
	if (!model)
		return 0;

	QDate d = QDate::currentDate();
	double val = 0;

	QSqlQuery m_Query;
	QString s_Query = QString("SELECT dt,val FROM tariff_history WHERE id=%1").arg(id);

	if (id >= 0)
	{
		if (!m_Query.exec(s_Query) || !m_Query.next())
		{
			ZMessager::Instance().Message(_Error, m_Query.lastError().text());
			return 0;
		}
		d = m_Query.value(0).toDate();
		val = m_Query.value(1).toDouble();
	}

	ZTariffsHistory dialog(this);
	if (dialog.getData(d, val) != 1)
		return 0;

	if (id == -1)
	{
		s_Query = QString("INSERT INTO tariff_history(tariff_id,dt,val) VALUES (%1, '%2', %3)").arg(currentId).arg(d.toString(DATE_FORMAT)).arg(val);
	}
	else
	{
		s_Query = QString("UPDATE tariff_history SET dt='%1', val=%2 WHERE id=%3").arg(d.toString(DATE_FORMAT)).arg(val).arg(id);
	}


	if (!m_Query.exec(s_Query))
	{
		ZMessager::Instance().Message(_Error, m_Query.lastError().text());
		return 0;
	}

	Update();
	return 1;
}

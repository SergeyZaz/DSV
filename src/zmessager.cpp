#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include "zmessager.h"

#define MSG_PIXMAP_SIZE 20

ZMsgModel::ZMsgModel(QObject* parent):
	QAbstractListModel(parent)
{
	headers.append(QObject::tr("Тип"));
	headers.append(QObject::tr("Время"));
	headers.append(QObject::tr("Текст"));

	connect(&timer, SIGNAL(timeout()), this, SLOT(timeout()));
	timer.start(1000);
	rows = 0;
}

QVariant ZMsgModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal)
		return headers.at(section);
	else
		return QString("");
}

QVariant ZMsgModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.column() >= headers.size())
		return QVariant();

	if (index.row() >= m_Data.size())
		return QVariant();

	if (index.column() == 0)
	{
		if (role == Qt::DecorationRole)
		{
			switch (m_Data.at(index.row()).type)
			{
			case _Warning:
				return QPixmap(":/images/msgbox5.png").scaled(MSG_PIXMAP_SIZE, MSG_PIXMAP_SIZE);
			case _Error:
				return QPixmap(":/images/msgbox6.png").scaled(MSG_PIXMAP_SIZE, MSG_PIXMAP_SIZE);
			case _CriticalError:
				return QPixmap(":/images/msgbox4.png").scaled(MSG_PIXMAP_SIZE, MSG_PIXMAP_SIZE);
			default:
				return QVariant();
			}
		}
	}
	if (role == Qt::DisplayRole)
	{
		QString tStr;
		switch (index.column())
		{
		case 0://код
			return QVariant(m_Data.at(index.row()).text);
		case 1://время
			return QVariant(m_Data.at(index.row()).time.toString("hh:mm:ss.zzz"));
		case 2://текст
			return QVariant(m_Data.at(index.row()).text);
		default:
			return QVariant();
		}
	}
	return QVariant();

}

void ZMsgModel::Update()
{
	beginResetModel();
	endResetModel();
}

void ZMsgModel::timeout()
{
	if (rows == m_Data.size())
		return;
	rows = m_Data.size();
	Update();
}

//////////////////////////////////////////////////////////////////////////////////////////////
ZMessager::ZMessager()
{ 
	pMsgList = NULL; 
	model.sortModel = new QSortFilterProxyModel;
	model.sortModel->setSourceModel(&model);
}

ZMessager::~ZMessager()
{
	delete model.sortModel;
}

ZMessager& ZMessager::Instance()
{
	static ZMessager singleton;
	return singleton;
}

void ZMessager::setWidget(QTableView* pW) 
{
	pMsgList = pW; 
	if (!pMsgList)
		return;
		
	pMsgList->setModel(model.sortModel);
	model.sortModel->setDynamicSortFilter(true);
	model.sortModel->setFilterKeyColumn(1);

	pMsgList->sortByColumn(1, Qt::DescendingOrder);
	pMsgList->setColumnWidth(0, MSG_PIXMAP_SIZE + 5);
	pMsgList->setColumnWidth(1, 100);
	pMsgList->setColumnWidth(2, 700);
	pMsgList->verticalHeader()->setDefaultSectionSize(20);
}

void ZMessager::Clear()
{
	model.Data()->clear();
	model.Update();
}

void ZMessager::Save()
{
	QString fileName;

	fileName = QFileDialog::getSaveFileName(NULL,
		QObject::tr("Сохранение списка сообщений в файл"),
		QDir::currentPath(),
		QObject::tr("Текстовый файл (*.txt)\n"));

	if (fileName.isEmpty())
		return;

	FILE* out = fopen(fileName.toLocal8Bit().data(), "wt");
	if (!out)
		return;

	char cType[128];
	int n = model.Data()->size();
	for (int i = 0; i < n; i++)
	{
		const OutputMsg* pMsg = &model.Data()->at(i);
		switch (pMsg->type)
		{
		case _Warning:
			strcpy(cType, "Предупреждение: ");
			break;
		case _Error:
			strcpy(cType, "Ошибка:");
			break;
		case _CriticalError:
			strcpy(cType, "Критическая ошибка:");
			break;
		default:
			strcpy(cType, "");
			break;
		}
		fprintf(out, "%s %s %s\n",
			pMsg->time.toString().toLocal8Bit().data(),
			cType,
			pMsg->text.toLocal8Bit().data());
	}
	fclose(out);
}

void ZMessager::Message(OutputMsgType type, const QString &txt, const QString title)
{
	if (!title.isEmpty())
	{
		QMessageBox::warning(NULL, title, txt, QMessageBox::Ok);
	}
	OutputMsg msg;
	msg.text = txt;
	msg.time = QTime::currentTime();
	msg.type = type;
	model.Data()->push_back(msg);
}

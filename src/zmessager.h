#pragma once

#include <QTableView>
#include <QTime>
#include <QStringList>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QTimer>

enum OutputMsgType
{
	_Warning,
	_Error,
	_CriticalError,
};

typedef struct
{
	OutputMsgType	type;
	QTime			time;
	QString			text;
} OutputMsg;

class ZMsgModel : public QAbstractListModel
{
	Q_OBJECT
	QStringList headers;
	QTimer		timer;
	uint		rows;
	QVariant headerData(int section, Qt::Orientation orientation,
		int role = Qt::DisplayRole) const;

public:
	ZMsgModel(QObject* parent = 0);
	int rowCount(const QModelIndex& parent) const { return m_Data.size(); }
	int columnCount(const QModelIndex& parent) const { return headers.size(); }
	QVariant data(const QModelIndex& index, int role) const;
	void Update();
	QList<OutputMsg>* Data() { return &m_Data; }

	QSortFilterProxyModel *sortModel;
private:
	QList<OutputMsg> m_Data;
public slots:
	void timeout();
};

class ZMessager
{
	QTableView* pMsgList;
	ZMsgModel	model;
	
public:
	ZMessager();
	~ZMessager();
	void setWidget(QTableView* pW);
	static ZMessager& Instance();
	void Clear();
	void Save();
	void Message(OutputMsgType type, const QString& txt, const QString title = "");
};



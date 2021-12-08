#pragma once
#include <QDialog>
#include <QDateTime>
#include <QComboBox>
#include <QSqlQuery>
#include "ui_zeditbaseform.h"

#define ADD_UNIC_CODE	-99

class ZEditAbstractForm : public QDialog
{
	Q_OBJECT
public:
	int						curEditId;
	QString					m_tbl;

	ZEditAbstractForm(QWidget *parent = 0, Qt::WindowFlags flags = 0):QDialog(parent, flags)
	{
		setModal(true);
	}
	virtual int init(const QString &tbl, int id)
	{
		m_tbl = tbl;
		curEditId = id;
		return 1;
	}
	virtual ~ZEditAbstractForm(){};
	virtual void setSectionsType(int){};
	void loadItemsToCombobox( QComboBox *comboBox, const QString &tableName, const QString &filter = "")
	{
		QString strQuery, txt = comboBox->currentText();

		// create query
		QSqlQuery query;

		// clear box
		comboBox->clear();

		if(tableName == "sections")
			strQuery = QString("SELECT id, name, parent FROM %1").arg(tableName);
		else
			strQuery = QString("SELECT id, name FROM %1").arg(tableName);

		if(!filter.isEmpty())
			strQuery += QString(" WHERE %1").arg(filter);

		strQuery += " ORDER BY name";

		int result = query.exec(strQuery);
	
		if (result)
		{
			while (query.next()) 
			{
				strQuery = query.value(1).toString().simplified();
				if(tableName == "sections")	//0-операционный поток, 1-инвестиционный поток, 2-финансовый поток
				{
					switch(query.value(2).toInt())
					{
					case 1:
						strQuery += tr(" (инвестиционный поток)");
						break;
					case 2:
						strQuery += tr(" (финансовый поток)");
						break;
					default:
						strQuery += tr(" (операционный поток)");
						break;
					}
				}
				comboBox->addItem(strQuery, query.value(0).toInt());
			}
		}

		if(!txt.isEmpty())
			comboBox->setCurrentIndex(comboBox->findText(txt));
	}

signals:	
	void errorQuery(const QDateTime &, long , const QString &);
	void needUpdateVal(int);
};

class ZEditBaseForm : public ZEditAbstractForm
{
	Q_OBJECT
	Ui::EditBaseForm ui;
	int fNeedComment;
	void closeEvent(QCloseEvent *event);
public:
	int init(const QString &tbl, int id);
	ZEditBaseForm(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	~ZEditBaseForm();
public slots:
	void applySlot();
	void textChangedSlot(const QString &);
};

#include <QMessageBox>
#include <QSettings>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDir>
#include <QTextCodec>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMdiSubWindow>
#include <QFileInfo>
#include <QDebug>

#include "zmainwindow.h"
#include "zpersons.h"
#include "zsmens.h"
#include "zorganisations.h"
#include "ztariffs.h"
#include "zgroups.h"
#include "zarchives.h"
#include "zpayments.h"
#include "zparsexlsxfile.h"
#include "zprotokol.h"
#include "zsettings.h"
#include "zmessager.h"
#include "zpayments2fio.h"
#include "zimportdata.h"
#include "znotes.h"

#define	CFG_FILE "config.ini"
#define	PROGRAMM_NAME "ДСВ"

ZMainWindow::ZMainWindow()
{
	ui.setupUi(this);
	setWindowTitle(PROGRAMM_NAME);

	ZMessager::Instance().setWidget(ui.msgList);

	connect(ui.actAbout, SIGNAL(triggered()), this, SLOT(slotAbout()));
	connect(ui.actAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

	connect(ui.actOpenFileXLSX, SIGNAL(triggered()), this,	SLOT(slotOpenXlsxFile()));

	connect(ui.actGroups, SIGNAL(triggered()), this,	SLOT(slotOpenGroupsDialog()));
	connect(ui.actPersons, SIGNAL(triggered()), this,	SLOT(slotOpenPersonsDialog()));
	connect(ui.actSmens, SIGNAL(triggered()), this,	SLOT(slotOpenSmensDialog()));
	connect(ui.actTariffs, SIGNAL(triggered()), this,	SLOT(slotOpenTariffsDialog()));
	connect(ui.actOrganisations, SIGNAL(triggered()), this,	SLOT(slotOpenOrganisationsDialog()));
	connect(ui.actArchivs, SIGNAL(triggered()), this, SLOT(slotOpenArchivsDialog()));
	connect(ui.actPayments, SIGNAL(triggered()), this, SLOT(slotOpenPaymentsDialog()));
	connect(ui.actPayments2fio, SIGNAL(triggered()), this, SLOT(slotOpenPayments2fioDialog()));
	connect(ui.actImportData, SIGNAL(triggered()), this, SLOT(slotOpenImportDataDialog()));
	connect(ui.actNotes, SIGNAL(triggered()), this, SLOT(slotOpenNotesDialog()));


	connect(ui.actProtokol, SIGNAL(triggered()), this,	SLOT(slotOpenProtokolDialog()));
	
	connect(ui.cmdCleanMsg, SIGNAL(clicked()), this, SLOT(slotCleanMsg()));
	connect(ui.cmdSaveMsg, SIGNAL(clicked()), this, SLOT(slotSaveMsg()));

	readSettings();

	if(readIniFile()==0)
		exit(0);
}

ZMainWindow::~ZMainWindow()
{
}

void ZMainWindow::closeEvent(QCloseEvent *event)
{
	ui.mdiArea->closeAllSubWindows();
	if (ui.mdiArea->currentSubWindow()) 
	{
		event->ignore();
	} 
	else 
	{
		writeSettings();
		event->accept();
	}
}

void ZMainWindow::slotAbout()
{
	QMessageBox::about(this, tr("О программе"),
		QString("Программа: \"%1\".<p>Версия 0.0.1. (Сборка: %2 %3) Автор: <a href=\"mailto:zaz@29.ru\">Zaz</a>")
		.arg( windowTitle() ).arg( __DATE__ ).arg( __TIME__ ));
}


void ZMainWindow::readSettings()
{
	QSettings settings("Zaz", "DSV");
	QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
	QSize size = settings.value("size", QSize(640, 480)).toSize();
/*
	qint64 d = settings.value("magic", 0).toLongLong();
	if (d == 0)
	{
		d = QDate::currentDate().toJulianDay();
		settings.setValue("magic", d);
	}
	if (d != 159753)
	{
		d = 30 - QDate::currentDate().toJulianDay() + d;
		if (d < 0)
		{
			QMessageBox::warning(this, "Внимание", "Ознакомительный период закончился, программа будет закрыта!");
			exit(0);
		}

		QMessageBox::warning(this, tr("Внимание"),
			QString("Вы используете ознакомительную версию, программа перестанет работать через %1 дней!").arg(d));
	}
*/
	move(pos);
	resize(size);
}

void ZMainWindow::writeSettings()
{
	QSettings settings("Zaz", "DSV");
	settings.setValue("pos", pos());
	settings.setValue("size", size());
}

int ZMainWindow::readIniFile()
{
	if (!QFile::exists(CFG_FILE)) 
    {
		ZMessager::Instance().Message(_CriticalError, tr("Отсутствует конфигурационный файл: %1").arg(QDir::currentPath() + "/" + CFG_FILE), tr("Ошибка"));
        return 0;
    }

    QSettings settings(CFG_FILE, QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

    db = QSqlDatabase::addDatabase(settings.value("Database/driver").toString());
    db.setDatabaseName(settings.value("Database/dbname").toString());
    db.setUserName(settings.value("Database/user").toString());
    db.setPassword(settings.value("Database/pwd").toString());
    db.setHostName(settings.value("Database/host").toString());
    db.setPort(settings.value("Database/port").toInt());

    db.setConnectOptions("connect_timeout=10"); 
    if (!db.open()) 
    {
		ZMessager::Instance().Message(_CriticalError, db.lastError().text(), tr("Ошибка"));
        return 0;
    }

	int size = settings.beginReadArray("Import");
	for (int i = 0; i < size; ++i) 
	{
		settings.setArrayIndex(i);
		ZSettings::Instance().importTags.insert(settings.value("key").toString(), settings.value("value").toString());
	}
	settings.endArray();
    return 1;
}
	
void ZMainWindow::slotUpdate()
{
	//обновление открытых окон
	foreach (QMdiSubWindow *window, ui.mdiArea->subWindowList()) 
	{
		ZMdiChild *pChild = dynamic_cast<ZMdiChild *>(window->widget());
		if (pChild)
		{
			pChild->blockSignals(true);
			pChild->reload();
			pChild->blockSignals(false);
			continue;
		}
		ZViewGroups *pViewGroups = dynamic_cast<ZViewGroups*>(window->widget());
		if (pViewGroups)
		{
			pViewGroups->blockSignals(true);
			pViewGroups->UpdateSlot(0);
			pViewGroups->blockSignals(false);
			continue;
		}
	}
}

void ZMainWindow::slotOpenXlsxFile()
{
	QString fileName = QFileDialog::getOpenFileName(this, "Выбор файла для импорта", "", "XLSX-файлы (*.xlsx);;Все файлы (*.*)");
	if (fileName.isEmpty()) 
		return;
	ZParseXLSXFile pFile;
	if(pFile.loadImportData(fileName))
	{
		slotUpdate();
	}
}

void ZMainWindow::slotOpenGroupsDialog()
{
	foreach (QMdiSubWindow *window, ui.mdiArea->subWindowList()) 
	{
		if (dynamic_cast<ZGroups*>(window->widget()))
		{
			ui.mdiArea->setActiveSubWindow(window);
			return;
		}
	}

	ZGroups*child = new ZGroups(this);
	connect(child, SIGNAL(needUpdate()), this,SLOT(slotUpdate()));
	ui.mdiArea->addSubWindow(child);
	child->setWindowTitleAndIcon(ui.actGroups->text(), ui.actGroups->icon());
	child->init("groups");
	child->show();
}

void ZMainWindow::slotOpenPersonsDialog()
{
	foreach (QMdiSubWindow *window, ui.mdiArea->subWindowList()) 
	{
		if (dynamic_cast<ZPersons *>(window->widget()))
		{
			ui.mdiArea->setActiveSubWindow(window);
			return;
		}
	}

	ZMdiChild *child = new ZPersons(this);
	connect(child, SIGNAL(needUpdate()), this,SLOT(slotUpdate()));
	ui.mdiArea->addSubWindow(child);
	child->setWindowTitleAndIcon(ui.actPersons->text(), ui.actPersons->icon());
	child->init("fio");
	child->show();
}

void ZMainWindow::slotOpenSmensDialog()
{
	foreach (QMdiSubWindow *window, ui.mdiArea->subWindowList()) 
	{
		if (dynamic_cast<ZSmens *>(window->widget()))
		{
			ui.mdiArea->setActiveSubWindow(window);
			return;
		}
	}

	ZMdiChild *child = new ZSmens(this);
	connect(child, SIGNAL(needUpdate()), this,SLOT(slotUpdate()));
	ui.mdiArea->addSubWindow(child);
	child->setWindowTitleAndIcon(ui.actSmens->text(), ui.actSmens->icon());
	child->init("smena");
	child->show();
}

void ZMainWindow::slotOpenTariffsDialog()
{
	foreach (QMdiSubWindow *window, ui.mdiArea->subWindowList()) 
	{
		if (dynamic_cast<ZTariffs*>(window->widget()))
		{
			ui.mdiArea->setActiveSubWindow(window);
			return;
		}
	}

	ZTariffs *child = new ZTariffs(this);
	connect(child, SIGNAL(needUpdate()), this,SLOT(slotUpdate()));
	ui.mdiArea->addSubWindow(child);
	child->init();
	child->show();
}

void ZMainWindow::slotOpenOrganisationsDialog()
{
	foreach (QMdiSubWindow *window, ui.mdiArea->subWindowList()) 
	{
		if (dynamic_cast<ZOrganisations *>(window->widget()))
		{
			ui.mdiArea->setActiveSubWindow(window);
			return;
		}
	}

	ZOrganisations* child = new ZOrganisations(this);
	connect(child, SIGNAL(needUpdate()), this,SLOT(slotUpdate()));
	ui.mdiArea->addSubWindow(child);
	child->setWindowTitleAndIcon(ui.actOrganisations->text(), ui.actOrganisations->icon());
	child->init("organisation");
	child->show();
}

void ZMainWindow::slotOpenArchivsDialog()
{
	foreach (QMdiSubWindow *window, ui.mdiArea->subWindowList()) 
	{
		if (dynamic_cast<ZArchives*>(window->widget()))
		{
			ui.mdiArea->setActiveSubWindow(window);
			return;
		}
	}

	ZMdiChild *child = new ZArchives(this);
	connect(child, SIGNAL(needUpdate()), this,SLOT(slotUpdate()));
	ui.mdiArea->addSubWindow(child);
	child->setWindowTitleAndIcon(ui.actArchivs->text(), ui.actArchivs->icon());
	child->init("import");
	child->show();
}

void ZMainWindow::slotOpenPaymentsDialog()
{
	foreach(QMdiSubWindow * window, ui.mdiArea->subWindowList())
	{
		if (dynamic_cast<ZPayments*>(window->widget()))
		{
			ui.mdiArea->setActiveSubWindow(window);
			return;
		}
	}

	ZMdiChild* child = new ZPayments(this);
	connect(child, SIGNAL(needUpdate()), this, SLOT(slotUpdate()));
	ui.mdiArea->addSubWindow(child);
	child->setWindowTitleAndIcon(ui.actPayments->text(), ui.actPayments->icon());
	child->init("payments");
	child->show();
}

void ZMainWindow::slotOpenPayments2fioDialog()
{
	foreach(QMdiSubWindow * window, ui.mdiArea->subWindowList())
	{
		if (dynamic_cast<ZPayments2fio*>(window->widget()))
		{
			ui.mdiArea->setActiveSubWindow(window);
			return;
		}
	}

	ZPayments2fio* child = new ZPayments2fio(this);
	connect(child, SIGNAL(needUpdate()), this, SLOT(slotUpdate()));
	ui.mdiArea->addSubWindow(child);
	//child->setWindowTitleAndIcon(ui.actPayments2fio->text(), ui.actPayments2fio->icon());
	child->init("payments2fio");
	child->show();
}


void ZMainWindow::slotOpenProtokolDialog()
{
	foreach(QMdiSubWindow * window, ui.mdiArea->subWindowList())
	{
		if (dynamic_cast<ZProtokol*>(window->widget()))
		{
			ui.mdiArea->setActiveSubWindow(window);
			return;
		}
	}

	QWidget* child = new ZProtokol(this);
	ui.mdiArea->addSubWindow(child);
	child->show();
}

void ZMainWindow::slotOpenImportDataDialog()
{
	foreach(QMdiSubWindow * window, ui.mdiArea->subWindowList())
	{
		if (dynamic_cast<ZImportData*>(window->widget()))
		{
			ui.mdiArea->setActiveSubWindow(window);
			return;
		}
	}

	ZMdiChild* child = new ZImportData(this);
	connect(child, SIGNAL(needUpdate()), this, SLOT(slotUpdate()));
	ui.mdiArea->addSubWindow(child);
	child->setWindowTitleAndIcon(ui.actImportData->text(), ui.actImportData->icon());
	child->init("import_data");
	child->show();
}

void ZMainWindow::slotOpenNotesDialog()
{
	foreach(QMdiSubWindow * window, ui.mdiArea->subWindowList())
	{
		if (dynamic_cast<ZNotes*>(window->widget()))
		{
			ui.mdiArea->setActiveSubWindow(window);
			return;
		}
	}

	ZMdiChild* child = new ZNotes(this);
	connect(child, SIGNAL(needUpdate()), this, SLOT(slotUpdate()));
	ui.mdiArea->addSubWindow(child);
	child->setWindowTitleAndIcon(ui.actNotes->text(), ui.actNotes->icon());
	child->init("notes2fio");
	child->show();
}

void ZMainWindow::slotUpdateAccountsVal(int account_id)
{
	QSqlQuery query, query2;
	int id;
	double val;
	QString stringQuery = "SELECT id FROM accounts";

	if(account_id!=-1)
		stringQuery += QString(" WHERE id=%1").arg(account_id);
	
	if (query.exec(stringQuery))
	{
		while (query.next()) 
		{
			id = query.value(0).toInt();
			val = 0;

			for(int i=0;i<2;i++)
			{
				//stringQuery = QString("SELECT sum(val) FROM operations WHERE type=%1 AND account=%2")
				stringQuery = QString("SELECT sum(val) FROM operations WHERE section IN (SELECT id FROM sections WHERE type=%1) AND account=%2")
					.arg(i) //Тип: 0-Поступление/1-Выплата/2-Перемещение
					.arg(id);
				if(query2.exec(stringQuery))
				{
					while (query2.next()) 
					{
						if(i==0)
							val += query2.value(0).toDouble();
						else
							val -= query2.value(0).toDouble();
					}
				}
				else
				{
				}
			}
			
			stringQuery = QString("UPDATE accounts SET val=%1 WHERE id=%2").arg(val, 0, 'f', 2).arg(id);
			if(!query2.exec(stringQuery))
				QMessageBox::critical(this, tr("Ошибка"), query2.lastError().text());
		}
	}	
	else
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
	}
}

void ZMainWindow::slotCleanMsg()
{
	ZMessager::Instance().Clear();
}

void ZMainWindow::slotSaveMsg()
{
	ZMessager::Instance().Save();
}

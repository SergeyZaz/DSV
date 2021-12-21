#pragma once

#include <QMainWindow>
#include <QSqlDatabase>
#include "ui_zmainwindow.h"

class ZMainWindow : public QMainWindow
{
	Q_OBJECT

	QSqlDatabase	db;
	Ui::ZMainWindow	ui;

public:
	ZMainWindow();
	~ZMainWindow();

protected:
	void	closeEvent(QCloseEvent *event);
	int		readIniFile();

private slots:
	void slotAbout();
	void slotOpenXlsxFile();
	void slotOpenGroupsDialog();
	void slotOpenPersonsDialog();
	void slotOpenSmensDialog();
	void slotOpenTariffsDialog();
	void slotOpenOrganisationsDialog();
	void slotOpenProtokolDialog();
	void slotOpenArchivsDialog();
	void slotOpenPaymentsDialog();
	void slotOpenPayments2fioDialog();
	void slotOpenImportDataDialog();
	void slotUpdate();
	void slotUpdateAccountsVal(int account_id);
	void slotCleanMsg();
	void slotSaveMsg();
	void slotOpenNotesDialog();

private:
	void readSettings();
	void writeSettings();

};

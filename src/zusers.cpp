#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QCryptographicHash>
#include "zusers.h"

ZUsers::ZUsers(QWidget* parent, Qt::WindowFlags flags): ZMdiChild(parent, flags)
{
}

void ZUsers::init(const QString &m_TblName)
{
	QList<int> hideColumns;
	QStringList headers;
	QList<int> cRem;
	
	hideColumns << 0;
	hideColumns << 2;
	headers << tr("id") << tr("Логин") << tr("Пароль") << tr("Тип");

	m_tbl->setTable(m_TblName, headers, cRem);	
	m_tbl->setCustomEditor(new ZUsersForm(this));
	
	QMap<int, QString>* pMap0 = new QMap<int, QString>;
	pMap0->insert(0, "Администратор");
	pMap0->insert(1, "Пользователь");
	m_tbl->setRelation(3, pMap0);

	m_tbl->init(hideColumns, 1, Qt::DescendingOrder);
}

/////////////////////////////////////////////////////////////////////////////////////////

ZUsersForm::ZUsersForm(QWidget* parent, Qt::WindowFlags flags) : ZEditAbstractForm(parent, flags)
{
	ui.setupUi(this);
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(applyChanges()));
	connect(ui.cmdSaveNew, SIGNAL(clicked()), this, SLOT(addNewSlot()));
}

ZUsersForm::~ZUsersForm() {}

int ZUsersForm::init(const QString& table, int id)
{
	ZEditAbstractForm::init(table, id);

	ui.txtLogin->setEnabled(id != 0);
	ui.cboMode->setEnabled(id != 0);
	ui.txtLogin->setText("");
	ui.txtPwd->setText("");
	ui.txtPwd2->setText("");
	ui.cboMode->setCurrentIndex(1);

	QString stringQuery = QString("SELECT name, pwd, mode FROM users WHERE id = %1")
		.arg(curEditId);

	// new record
	if (curEditId == ADD_UNIC_CODE)
	{
		return true;
	}

	// execute request
	QSqlQuery query;
	bool result = query.exec(stringQuery);
	if (result)
	{
		if (query.next())
		{
			ui.txtLogin->setText(query.value(0).toString());
			ui.txtPwd->setText(query.value(1).toString());
			ui.cboMode->setCurrentIndex(query.value(2).toInt());
		}
	}
	else
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
	}

	return result;
}

void ZUsersForm::addNewSlot()
{
	curEditId = ADD_UNIC_CODE;
	applyChanges();
}

void ZUsersForm::applyChanges()
{
	QString text, stringQuery;

	if (curEditId == ADD_UNIC_CODE)
		stringQuery = QString("INSERT INTO users (name, pwd, mode) VALUES (?, ?, ?)");
	else
		stringQuery = QString("UPDATE users SET name=?, pwd=?, mode=? WHERE id=%1").arg(curEditId);

	QSqlQuery query;
	query.prepare(stringQuery);

	text = ui.txtLogin->text();
	if (text.isEmpty())
	{
		QMessageBox::critical(this, tr("Ошибка"), tr("Введите логин!"));
		return;
	}
	query.addBindValue(text);

	text = ui.txtPwd->text();
	if (text.size() < 6)
	{
		QMessageBox::critical(this, tr("Ошибка"), tr("Пароль должен быть не менее 6 символов!"));
		return;
	}
	if (text != ui.txtPwd2->text())
	{
		QMessageBox::critical(this, tr("Ошибка"), tr("Введенные пароли не совпадают!"));
		return;
	}

	text = QCryptographicHash::hash(text.toLocal8Bit(), QCryptographicHash::Md5).toHex();

	query.addBindValue(text);
	query.addBindValue(ui.cboMode->currentIndex());

	if (!query.exec())
	{
		QMessageBox::critical(this, tr("Ошибка"), query.lastError().text());
		return;
	}

	accept();
}

bool CheckPwd(const QString& login, const QString& psw, int *pType)
{
	QSqlQuery query;
	if (!query.exec(QString("SELECT pwd, mode FROM users WHERE name = '%1'").arg(login)))
	{
		QMessageBox::critical(NULL, QString("Ошибка"), query.lastError().text());
		return false;
	}
	
	if (!query.next())
	{
		QMessageBox::critical(NULL, QString("Ошибка"), QString("Данные отсутствуют!"));
		return false;
	}

	if (pType)
		(*pType) = query.value(1).toInt();

	QString txt = QCryptographicHash::hash(psw.toLocal8Bit(), QCryptographicHash::Md5).toHex();
	return (query.value(0).toString() == txt);
}

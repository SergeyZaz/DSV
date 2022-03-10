#include "zauth.h"
#include "zusers.h"
#include "zmessager.h"
#include "zsettings.h"

ZAuthForm::ZAuthForm(QWidget* parent, Qt::WindowFlags flags)
{
	ui.setupUi(this);

	ui.txtLogin->setText(ZSettings::Instance().m_UserName);
	ui.txtPwd->setText(ZSettings::Instance().m_Password);

	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(applyChanges()));

	if (!ZSettings::Instance().m_UserName.isEmpty())
		ui.txtPwd->setFocus();
}

ZAuthForm::~ZAuthForm() {}

int ZAuthForm::execute()
{
	if (!ZSettings::Instance().m_Password.isEmpty())
	{
		applyChanges();
		if (!ui.txtPwd->text().isEmpty())
			return 1;
	}
	return exec();
}

void ZAuthForm::applyChanges()
{
	int type;
	if (!CheckPwd(ui.txtLogin->text(), ui.txtPwd->text(), &type))
	{
		ZMessager::Instance().Message(_CriticalError, tr("Ошибка авторизации! Проверьте имя пользователя и пароль!"), tr("Ошибка"));
		ui.txtPwd->setText("");
		ui.txtPwd->setFocus();
		return;
	}

	ZSettings::Instance().m_UserType = type;
	ZSettings::Instance().m_UserName = ui.txtLogin->text();
	accept();
}


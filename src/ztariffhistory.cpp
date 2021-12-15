#include "ztariffhistory.h"

ZTariffsHistory::ZTariffsHistory(QWidget* parent, Qt::WindowFlags flags)// : QDialog(parent, flags)
{
	ui.setupUi(this);
	ui.dateEdit->setCalendarPopup(true);
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(applyChanges()));
}

ZTariffsHistory::~ZTariffsHistory(){}

int ZTariffsHistory::getData(QDate& d, double &val)
{
	ui.dateEdit->setDate(d);
	ui.spinBonus->setValue(val);

	if (exec() != Accepted)
		return 0;

	d = ui.dateEdit->date();
	val = ui.spinBonus->value();
	return 1;
}

void ZTariffsHistory::applyChanges()
{
	accept();
}

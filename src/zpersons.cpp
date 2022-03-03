#include <QMessageBox>
#include "zpersons.h"
#include "zmessager.h"

ZPersons::ZPersons(QWidget* parent, Qt::WindowFlags flags) : ZMdiChild(parent, flags)
{
}


void ZPersons::init(const QString &m_TblName)
{
	QList<int> hideColumns;
	QStringList headers;
	QList<int> cRem;
	
	hideColumns << 0;
	headers <<  tr("id") << tr("ФИО") << tr("Комментарий");

	m_tbl->setTable(m_TblName, headers, cRem);
	m_tbl->init(hideColumns);
	m_tbl->moveSection(3, 5);

	headers.clear();
	headers << "Объединить" << "Очистить комментарий";
	setContextMenuForTbl(headers);
}

void ZPersons::execCustomAction(const QString& text)
{
	QModelIndexList listIndxs = m_tbl->getTblView()->selectionModel()->selectedRows();
	QMap<int, QString> items;
	int tId;
	foreach(QModelIndex index, listIndxs)
	{
		QModelIndex indx = m_tbl->getSortModel()->mapToSource(index);
		tId = m_tbl->getModel()->data(m_tbl->getModel()->index(indx.row(), 0)).toInt(); // id должен быть 0-м столюцом!!!
		if (tId != 0 && items.value(tId).isEmpty())
			items.insert(tId, m_tbl->getModel()->data(m_tbl->getModel()->index(indx.row(), 1)).toString());
	}

	if (items.size() < 2)
		return;

	QSqlQuery m_Query;
	QString s_Query;

	if (text == "Объединить")
	{
		QStringList sItems = items.values();
		bool ok;
		QString str = getZItem(this, QString("Внимание!"),
			QString("Выберите ФИО которое будет оставлено (остальные будут удалены!):"), sItems, 0, true, &ok, Qt::MSWindowsFixedSizeDialogHint);
		if (!ok || str.isEmpty())
			return;

		QApplication::setOverrideCursor(Qt::WaitCursor);

		tId = items.key(str); //кто остается
		items.remove(tId);

		sItems.clear();
		sItems << "import_data" << "payments2fio" << "notes2fio";

		QMap<int, QString>::const_iterator iT = items.constBegin();
		while (iT != items.constEnd())
		{
			ok = true;

			int id = iT.key();
			QString t_name = iT.value();
			++iT;

			for (int i = 0; i < sItems.size(); i++)
			{
				s_Query = tr("UPDATE %1 SET fio=%2 WHERE fio=%3").arg(sItems[i]).arg(tId).arg(id);

				if (!m_Query.exec(s_Query))
				{
					ok = false;

					str = "При объединении ";
					switch (i)
					{
					case 0:
						str += "смен ";
						break;
					case 1:
						str += "выплат ";
						break;
					case 2:
						str += "заметок ";
						break;
					default:
						continue;
					}

					str += "для '" + t_name + "' возникла ошибка: " + m_Query.lastError().text();
					ZMessager::Instance().Message(_CriticalError, str);
				}
			}

			if (!ok)
				continue;

			s_Query = tr("DELETE FROM fio WHERE id = %1").arg(id);
			if (!m_Query.exec(s_Query))
				ZMessager::Instance().Message(_CriticalError, m_Query.lastError().text());
			else
			{
				str = QString("'%1' удален!").arg(t_name);
				ZMessager::Instance().Message(_Warning, str);
			}
		}
	}

	else if (text == "Очистить комментарий")
	{
		if (QMessageBox::question(this, tr("Внимание"), tr("Вы действительно хотите удалить выделенные комментарии?"), tr("Да"), tr("Нет"), QString::null, 0, 1) != 0)
			return;

		QApplication::setOverrideCursor(Qt::WaitCursor);

		s_Query = tr("UPDATE fio SET comment='' WHERE id IN (");
		QMap<int, QString>::const_iterator iT = items.constBegin();
		while (iT != items.constEnd())
		{
			s_Query += QString("%1,").arg(iT.key());
			++iT;
		}
		s_Query.chop(1);
		s_Query += ")";
		
		if (!m_Query.exec(s_Query))
			ZMessager::Instance().Message(_CriticalError, m_Query.lastError().text());
	}

	emit needUpdate();
	QApplication::restoreOverrideCursor();
}

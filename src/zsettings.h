#pragma once

#include <QMap>
#include <QString>
#include <QDate>

class ZSettings
{
public:
    ZSettings() { m_UserType = -1; f_ReadOnly = false; };
    QMap<QString, QString> importTags;
    QDate   m_CloseDate;
    int m_UserType;// 0-�������������, 1-������������
    bool f_ReadOnly;
    QString m_UserName;
	static ZSettings& Instance();
};



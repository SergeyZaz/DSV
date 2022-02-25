#pragma once

#include <QMap>
#include <QString>

class ZSettings
{
public:
    ZSettings() { m_UserType = -1; };
    QMap<QString, QString> importTags;
    int m_UserType;// 0-администратор, 1-пользователь
    QString m_UserName;
	static ZSettings& Instance();
};



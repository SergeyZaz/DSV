#pragma once

#include <QMap>
#include <QString>

class ZSettings
{
public:
    ZSettings() { m_UserType = -1; };
    QMap<QString, QString> importTags;
    int m_UserType;// 0-�������������, 1-������������
    QString m_UserName;
	static ZSettings& Instance();
};



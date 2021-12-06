#pragma once

#include <QMap>
#include <QString>

class ZSettings
{
public:
    ZSettings() {};
    QMap<QString, QString> importTags;
	static ZSettings& Instance();
};



#ifndef ZSETTINGS_H
#define ZSETTINGS_H

#include <QMap>
#include <QString>

class ZSettings
{
public:
    ZSettings() {};
    QMap<QString, QString> importTags;
	static ZSettings& Instance();
};

#endif

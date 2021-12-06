#pragma once
#include <QMap>

class ZParseFile
{
	QString curSchet;
	bool fParseStart;
	QMap<QString, QString> mapData;
	void message(const QString& txt);
public:
	ZParseFile(){fParseStart = false;}
	~ZParseFile(){};
	int parseData(const QString &data);
	bool loadFile(const QString &fileName);
};



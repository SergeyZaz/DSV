#pragma once

#include <QVector>
#include <QVariant>

class ZParseXLSXFile
{
	int maxRow;
	int maxCol;
	QVector< QVector<QVariant> > m_Data;
	int insertData(uint key);
public:
	ZParseXLSXFile() { maxRow = -1; maxCol = -1;}
	~ZParseXLSXFile(){};
	bool loadFile(const QString &fileName);
};



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
	ZParseXLSXFile();
	~ZParseXLSXFile();
	bool loadFile(const QString& fileName);
	bool loadImportData(const QString& fileName);
	bool loadPayments(const QString& fileName);
};



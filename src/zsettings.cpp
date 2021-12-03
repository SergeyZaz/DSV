#include "zsettings.h"

ZSettings& ZSettings::Instance()
{
	static ZSettings singleton;
	return singleton;
}

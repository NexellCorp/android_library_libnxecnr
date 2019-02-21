#include <cutils/properties.h>
#include "nx-ecnr.h"

static const char *EN_ECNR_PROP_KEY = "persist.enable.ecnr";

void ecnr_init(void)
{
	char cmd[50];

	sprintf(cmd, "setprop %s 0", EN_ECNR_PROP_KEY);
	system(cmd);

	ecnr_is_enabled();
}

bool set_ecnr(bool enable)
{
	int ret;
	char cmd[50];

	ret = sprintf(cmd, "setprop %s %d", EN_ECNR_PROP_KEY, enable);
	if (ret < 0) {
		printf("Can't set ecnr prop %d", ret);
	} 
	system(cmd);
	
	ecnr_is_enabled();

	return ret;
}

bool ecnr_is_enabled(void)
{
	int ret = 0;

	ret = property_get_bool(EN_ECNR_PROP_KEY, 0);
	if (ret < 0) {
		printf("Can't set ecnr prop %d", ret);
	} 

	return ret;
}

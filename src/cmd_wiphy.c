
#include "user_interface.h"

#include "console.h"

static int do_wiphy (int argc, const char* const* argv)
{
	int ret = -1;
	enum phy_mode mode;
	if (argc == 1)
	{
		char pmode = '?';
		mode = wifi_get_phy_mode();
		ret = 0;
		switch (mode)
		{
		case PHY_MODE_11B: pmode = 'B'; break;
		case PHY_MODE_11G: pmode = 'G'; break;
		case PHY_MODE_11N: pmode = 'N'; break;
		default: ret = -1;
		}
		console_printf("phy mode: 802.11%c\n", pmode);
	}
	else if (argc == 2)
	{
		ret = 0;
		switch (argv[1][0])
		{
			case 'b':
			case 'B': mode = PHY_MODE_11B; break;
			case 'g':
			case 'G': mode = PHY_MODE_11G; break;
			case 'n':
			case 'N': mode = PHY_MODE_11N; break;
			default: ret = -1;
		}
		if (ret != -1)
		{
			ret = wifi_set_phy_mode(mode)? 0: -1;
			do_wiphy(1, NULL); // display state
		}
	}
	return ret;
}

const char sleep_none[] = "none";
const char sleep_light[] = "light";
const char sleep_modem[] = "modem";

static int do_physleep (int argc, const char* const* argv)
{
	int ret = -1;
	enum sleep_type mode;
	if (argc == 1)
	{
		const char* pmode = "?";
		mode = wifi_get_sleep_type();
		ret = 0;
		switch (mode)
		{
		case NONE_SLEEP_T: pmode = sleep_none; break;
		case LIGHT_SLEEP_T: pmode = sleep_light; break;
		case MODEM_SLEEP_T: pmode = sleep_modem; break;
		default: ret = -1;
		}
		console_printf("phy sleep mode: %s\n", pmode);
	}
	else if (argc == 2)
	{
		ret = 0;
		if (stricmp(argv[1], sleep_none) == 0)
			mode = NONE_SLEEP_T;
		else if (stricmp(argv[1], sleep_light) == 0)
			mode = LIGHT_SLEEP_T;
		else if (stricmp(argv[1], sleep_modem) == 0)
			mode = MODEM_SLEEP_T;
		else
			ret = -1;
		if (ret != -1)
		{
			ret = wifi_set_sleep_type(mode)? 0: -1;
			do_physleep(1, NULL); // display state
		}
	}
	return ret;
}

CONSOLE_CMD(wiphy, -1, -1, 
	    do_wiphy, NULL, NULL, 
	    "get/select wifi phy mode: B/G/N");

CONSOLE_CMD(physleep, -1, -1, 
	    do_physleep, NULL, NULL, 
	    "get/select wifi sleep mode: none/light/modem");

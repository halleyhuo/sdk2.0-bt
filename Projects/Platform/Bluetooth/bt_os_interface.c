
#include "type.h"
#include "bt_platform_interface.h"
#include "bt_config.h"
#include "freertos.h"


static void * osMalloc(uint32_t size)
{
	return pvPortMalloc(size);
}

static void osFree(void * ptr)
{
	vPortFree(ptr);
}

PLATFORM_INTERFACE_OS_T		pfiOS = {
	osMalloc,
	osFree,
	NULL,
	NULL
};


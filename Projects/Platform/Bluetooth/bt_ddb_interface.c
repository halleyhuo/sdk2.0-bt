


#include "app_config.h"
#include "bt_stack_api.h"
#include "bt_platform_interface.h"
#include "bt_trust_list.h"

#define BT_DDB_INTERFACE_DBG


#ifdef BT_DDB_INTERFACE_DBG
	#define BT_DDBI_DBG			DBG
#else
	#define BT_DDBI_DBG(x)
#endif

static bool OpenBtRecord(const uint8_t * localBdAddr);
static bool CloseBtRecord(void);
static bool AddBtRecord(const BT_DB_RECORD * btDbRecord);
static bool DeleteBtRecord(const uint8_t * bdAddr);
static bool FindBtRecord(const uint8_t * bdAddr, BT_DB_RECORD * btDbRecord);
static bool FindBtRecordByIndex(uint8_t index, BT_DB_RECORD * btDbRecord);




PLATFORM_INTERFACE_BT_DDB_T	pfiBtDdb = {
		OpenBtRecord,
		CloseBtRecord,
		AddBtRecord,
		DeleteBtRecord,
		FindBtRecord,
		FindBtRecordByIndex
};


static bool OpenBtRecord(const uint8_t * localBdAddr)
{
	BT_DDBI_DBG("OpenBtRecord localBdAddr = %02x:%02x:%02x:%02x:%02x:%02x\n", 
				localBdAddr[0],
				localBdAddr[1],
				localBdAddr[2],
				localBdAddr[3],
				localBdAddr[4],
				localBdAddr[5]);

	if(localBdAddr == NULL)
		return FALSE;

	if(!InitTrustList(localBdAddr, 0))
		return FALSE;

	return TRUE;
}

static bool CloseBtRecord(void)
{
	BT_DDBI_DBG("CloseBtRecord\n");

	if(!DeinitTrustList())
		return FALSE;

	return TRUE;
}

static bool AddBtRecord(const BT_DB_RECORD * btDbRecord)
{
	BT_DDBI_DBG("AddBtRecord bdAddr = %02x:%02x:%02x:%02x:%02x:%02x ; trusted = %d \n",
					btDbRecord->bdAddr[0],
					btDbRecord->bdAddr[1],
					btDbRecord->bdAddr[2],
					btDbRecord->bdAddr[3],
					btDbRecord->bdAddr[4],
					btDbRecord->bdAddr[5],
					btDbRecord->trusted
					);
	if(!InsertRecordToTrustList(btDbRecord, btDbRecord->trusted))
		return FALSE;

	return TRUE;
}

static bool DeleteBtRecord(const uint8_t * remoteBdAddr)
{
	BT_DDBI_DBG("DeleteBtRecord remoteBdAddr = %02x:%02x:%02x:%02x:%02x:%02x\n", 
					remoteBdAddr[0],
					remoteBdAddr[1],
					remoteBdAddr[2],
					remoteBdAddr[3],
					remoteBdAddr[4],
					remoteBdAddr[5]
					);
	if(!RemoveRecordFromTrustList(remoteBdAddr))
		return FALSE;

	return TRUE;
}

static bool FindBtRecord(const uint8_t * remoteBdAddr, BT_DB_RECORD * btDbRecord)
{
	BT_DDBI_DBG("FindBtRecord...\n");

	if(remoteBdAddr == NULL || btDbRecord == NULL)
	{
		BT_DDBI_DBG("Params error!\n");
		return FALSE;
	}

	if(!FindRecordInTrustList(remoteBdAddr, btDbRecord))
	{
		BT_DDBI_DBG("Failed\n");
		return FALSE;
	}

	BT_DDBI_DBG("Success. Trusted flag is %d\n", btDbRecord->trusted);
	return TRUE;
}

static bool FindBtRecordByIndex(uint8_t index, BT_DB_RECORD * btDbRecord)
{
	BT_DDBI_DBG("FindBtRecordByIndex index = %d ...\n", index);

	if(btDbRecord == NULL)
	{
		BT_DDBI_DBG("Params error!\n");
		return FALSE;
	}
	
	if(!FindRecordInTrustListByIndex(index, btDbRecord))
	{
		BT_DDBI_DBG("Failed\n");
		return FALSE;
	}

	BT_DDBI_DBG("Success. Trusted flag is %d\n", btDbRecord->trusted);

	return TRUE;
}




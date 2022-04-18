#include "stdint.h"
#include "ustdio.h"
#include "string.h"
#include "flash.h"
#include "iap.h"
#include "mylib.h"
#include "db.h"

/********************************************************************************************************/
#define flash_mem_write(addr, buff, cunt)       flash_write(flash_cfg, addr, buff,cunt)
#define flash_mem_read(addr, buff, cunt)        flash_read(flash_cfg, addr, buff,cunt)

//数据首地址和数据长度
const uint32_t DB_INFO[][2] = 
{ 
    {DB_APPBAK_ADDR,        sizeof(IAP_AppBak_stk)},      \

};

int db_read(void *datp, DB_TYPE type)
{
    uint32_t addr;
    uint32_t len;
    bool   err;

    addr = DB_INFO[type][0];
    len  = DB_INFO[type][1];

    err = flash_mem_read(addr, datp, len);

    if(FALSE == err)
    {
        memset(datp, 0, len);
        return(FALSE);
    }
    return(TRUE);
}

void db_write(const void *datp, DB_TYPE type)
{
    uint32_t addr;
    uint32_t len;

    addr = DB_INFO[type][0];
    len  = DB_INFO[type][1];

    flash_mem_write(addr, datp, len);
}

void db_init(void)
{

}

/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/

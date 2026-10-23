/*
 * The yn overlay's access to the network settings file, below its screens:
 * Sony's netcnf and netcnfif IOP modules loaded and unloaded
 * (YnNetcnf_LoadModules, YnNetcnf_Close, Yn_LoadModule, Yn_UnloadModule),
 * the file's entry lists read through netcnfif (YnNetcnf, entry types 0, 1
 * and 2), the conversions between the screens' YnConfig and netcnf's
 * interface and device data, the matching of a YnConfig against the saved
 * combinations, the attached network devices (gNetDevs), and the dotted
 * address and Shift-JIS/UTF-8 text conversions the entries need.
 *
 * yn_main.c's screens call these; nothing here draws. The file owns its
 * .data (the modules' argument lists) and .rodata (the module paths and
 * formats).
 */
#include "common.h"
#include "sif.h"
#include "yn.h"
#include "net_save.h"
#include "text.h"
#include "net_devices.h"
#include "libkernl.h"
#include "netcnfif.h"

/* main: libc and the IOP module calls */
void *memset(void *dst, s32 c, u32 n);
s32 sprintf(char *dst, char *fmt, ...);
s32 strcmp(char *a, char *b);
char *strcpy(char *dst, char *src);
/* sub_main: the network devices */
s32 NetDev_IsKnownName(char *vendor, char *product);
/* Sony's netcnfif (netcnfif.s) */
s32 yn_entry(void);
s32 func_00AC8128(void);
void sceNetcnfifGetList(char *path, s32 type, NetcnfEntry *list, s32 count);
s32 sceNetcnfifLoadEntry(char *path, s32 type, char *usrName, NetcnfData *data);
void func_00AC8DC0(void);
void sceNetcnfifDataInit(NetcnfData *data);

/* The busy wait in the IRX loaders' retry loops: the original has eight
 * nops as the loop body. */
#define YN_WAIT() __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop")
#define YN_WAIT7() __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop")

extern NetDevice *gNetDevCur;
#define gNetDevice (gNetDevCur)   /* sub_main: the device in use */

char gYnNetcnfArgs[128] = {
#include "yn/00AD21E0.inc"
};
char gYnNetcnfifArgs[32] = {
#include "yn/00AD2260.inc"
};
/*
 * Loads the netcnf and netcnfif IOP modules from the disc with
 * gYnNetcnfArgs and gYnNetcnfifArgs, then retries yn_entry until it
 * succeeds. Returns 0; n is unused.
 */
s32 YnNetcnf_LoadModules(YnNetcnf *n)
{
    Yn_LoadModule("cdrom0:\\IOP\\MODULES\\NETICON\\NETCNF.IRX;1", 127, gYnNetcnfArgs, 0, 0);
    Yn_LoadModule("cdrom0:\\IOP\\MODULES\\NETICON\\NETCNFIF.IRX;1", 22, gYnNetcnfifArgs, 0, 0);
    func_00AC8128();
    while (yn_entry() < 0) {
    }
    return 0;
}

/*
 * Allocates netcnfif's work area (each call retried or waited on) and
 * keeps the result in n->result. Returns -1 on failure, else clears the
 * no-decode flag and returns 0.
 */
s32 YnNetcnf_Open(YnNetcnf *n)
{
    while (sceNetcnfifAllocWorkarea(4096) < 0) {
    }
    while (sceNetcnfifCheck() != 0) {
    }
    while (sceNetcnfifGetResult(&n->result) < 0) {
    }
    if (n->result < 0) {
        return -1;
    }
    sceNetcnfifSetFNoDecode(0);
    return 0;
}

/*
 * Frees netcnfif's work area, then stops and unloads the netcnfif and
 * netcnf modules when they are loaded. Returns 0.
 */
s32 YnNetcnf_Close(YnNetcnf *n)
{
    s32 id;

    while (sceNetcnfifFreeWorkarea() < 0) {
    }
    while (sceNetcnfifCheck() != 0) {
    }
    func_00AC8DC0();
    id = sceSifSearchModuleByName("Netcnf_Interface");
    if (0 <= id) {
        Yn_UnloadModule(id);
    }
    id = sceSifSearchModuleByName("NET_configuration");
    if (0 <= id) {
        Yn_UnloadModule(id);
    }
    return 0;
}

/*
 * Returns the usrName of the type 1 entry whose sysName is name (seen: an
 * entry's attachIfc), or NULL.
 */
char *YnNetcnf_FindUsrName(YnNetcnf *n, char *name)
{
    s32 i;

    for (i = 0; i < n->count[1]; i++) {
        if (strcmp(n->list1[i].sysName, name) == 0) {
            return n->list1[i].usrName;
        }
    }
    return NULL;
}

/*
 * Fills netcnf interface data from cfg: PPPoE (user, password, MTU 1454),
 * DHCP or a fixed address, given or negotiated DNS. name gets cfg->name
 * through Yn_SjisToUtf8.
 */
void Yn_ConfigToIfc(YnConfig *cfg, NetcnfData *data, char *name)
{
    sceNetcnfifDataInit(data);
    data->auth = 0;
    if (cfg->ifcType3 == 1) {
        data->ifcType = 3;
    } else {
        data->ifcType = 1;
    }
    if (cfg->noPppoe == 0) {
        data->pppoe = 1;
        data->dhcp = 0;
        strcpy(data->authName, cfg->authName);
        strcpy(data->authKey, cfg->authKey);
        strcpy(data->peerName, "*");
        data->fAuth = 1;
        data->auth = 4;
        data->prcNego = 0;
        data->accNego = 0;
        data->accmNego = 0;
        data->mtu = 1454;
        data->ifcType = 2;
        data->ifcIdleTimeout = 0;
    } else {
        data->pppoe = 255;
        if (cfg->fixedIp == 0) {
            data->dhcp = 1;
            strcpy(data->dhcpHostname, cfg->dhcpHostname);
        } else {
            data->dhcp = 0;
            Yn_FormatAddress(data->address, cfg->address);
            Yn_FormatAddress(data->netmask, cfg->netmask);
            Yn_FormatAddress(data->gateway, cfg->gateway);
        }
    }
    if (cfg->dnsSet != 0) {
        if (Yn_IsAddressSet(cfg->dns1) != 0) {
            Yn_FormatAddress(data->dns1Address, cfg->dns1);
        }
        if (Yn_IsAddressSet(cfg->dns2) != 0) {
            Yn_FormatAddress(data->dns2Address, cfg->dns2);
        }
        if (data->dns1Address[0] == 0) {
            memset(data->dns1Address, 0, 256);
            memset(data->dns2Address, 0, 256);
            data->dns1Nego = 1;
            data->dns2Nego = 1;
        }
        if (data->dns2Address[0] == 0) {
            memset(data->dns2Address, 0, 256);
        }
    } else if (data->dhcp != 1) {
        data->dns1Nego = 1;
        data->dns2Nego = 1;
    }
    Yn_SjisToUtf8(name, cfg->name);
}

/*
 * Fills cfg (cleared first) from interface data: PPPoE user and password,
 * or the DHCP host name or fixed addresses, and the DNS servers when set;
 * cfg->name gets name through Yn_Utf8ToSjis.
 */
void Yn_IfcToConfig(YnConfig *cfg, NetcnfData *data, char *name)
{
    memset(cfg, 0, 2592);
    cfg->ifcType = data->ifcType;
    if (data->pppoe == 1) {
        strcpy(cfg->authName, data->authName);
        strcpy(cfg->authKey, data->authKey);
    } else {
        cfg->noPppoe = 1;
        if (data->dhcp != 255) {
            if (data->dhcp == 1) {
                strcpy(cfg->dhcpHostname, data->dhcpHostname);
            } else {
                cfg->fixedIp = 1;
                Yn_ParseAddress((u8 *)data->address, cfg->address);
                Yn_ParseAddress((u8 *)data->netmask, cfg->netmask);
                Yn_ParseAddress((u8 *)data->gateway, cfg->gateway);
            }
        }
    }
    if (data->dns1Address[0] != 0) {
        cfg->dnsSet = 1;
        Yn_ParseAddress((u8 *)data->dns1Address, cfg->dns1);
        Yn_ParseAddress((u8 *)data->dns2Address, cfg->dns2);
    }
    if (Yn_IsAddressSet(cfg->dns1) == 0) {
        cfg->dnsSet = 0;
        cfg->dns1[0] = 0;
        cfg->dns2[0] = 0;
        cfg->dns1[1] = 0;
        cfg->dns2[1] = 0;
        cfg->dns1[2] = 0;
        cfg->dns2[2] = 0;
        cfg->dns1[3] = 0;
        cfg->dns2[3] = 0;
    }
    Yn_Utf8ToSjis(cfg->name, name);
}

/*
 * Fills netcnf device data from cfg: vendor, product, devType (3 when
 * ifcType3 is 1, else 1) and phyConfig from cfg's ethernet mode.
 */
void Yn_ConfigToDev(YnConfig *cfg, NetcnfData *data)
{
    sceNetcnfifDataInit(data);
    strcpy(data->vendor, cfg->vendor);
    strcpy(data->product, cfg->product);
    if (cfg->ifcType3 == 1) {
        data->devType = 3;
    } else {
        data->devType = 1;
    }
    switch (cfg->phyConfig) {
    case 0:
    default:
        data->phyConfig = 1;
        break;
    case 1:
        data->phyConfig = 2;
        break;
    case 2:
        data->phyConfig = 3;
        break;
    case 3:
        data->phyConfig = 5;
        break;
    case 4:
        data->phyConfig = 6;
        break;
    }
}

/*
 * Copies the device data's vendor and product into cfg and maps its
 * phyConfig back to cfg's ethernet mode (0 for auto or unknown).
 */
void Yn_DevToConfig(YnConfig *cfg, NetcnfData *data)
{
    strcpy(cfg->vendor, data->vendor);
    strcpy(cfg->product, data->product);
    switch (data->phyConfig) {
    case 1:
    default:
        cfg->phyConfig = 0;
        break;
    case 2:
        cfg->phyConfig = 1;
        break;
    case 3:
        cfg->phyConfig = 2;
        break;
    case 5:
        cfg->phyConfig = 3;
        break;
    case 6:
        cfg->phyConfig = 4;
        break;
    }
}

/*
 * Fills one YnCombo per type 0 entry: its data, vendor and product, the
 * device kind (Yn_GetDeviceKind) and the attached device's index
 * (Yn_FindDevice). Leaves type 1 selected.
 */
void Yn_BuildCombos(YnNetcnf *n, YnCombo *combo)
{
    s32 i;

    YnNetcnf_SetType(n, 1);
    for (i = 0; i < n->count[0]; i++) {
        combo->vendor = n->data[i].vendor;
        combo->product = n->data[i].product;
        combo->data = &n->data[i];
        if (combo->vendor == NULL || combo->product == NULL) {
            combo->kind = 0;
            combo->device = -2;
        } else {
            combo->kind = Yn_GetDeviceKind(combo->vendor, combo->product);
            combo->device = Yn_FindDevice(combo->vendor, combo->product);
        }
        combo++;
    }
}

/*
 * Returns the index of the first of the n combos whose entry matches cfg
 * (device, DHCP host, PPPoE user and password, address, netmask, gateway
 * and both DNS servers), or -1.
 */
s32 Yn_FindCombination(YnConfig *cfg, YnCombo *combo, s32 n)
{
    u32 addr;
    char name[512];
    struct {
        NetcnfData data;
        u8 spare[48];
    } ifcx;
    NetcnfData dev;
    NetcnfData *entry;
    s32 i;

    name[0] = 0;
    Yn_ConfigToIfc(cfg, &ifcx.data, name);
    Yn_ConfigToDev(cfg, &dev);
    for (i = 0; i < n; i++, combo++) {
        entry = combo->data;
        if (entry != NULL && strcmp(entry->vendor, dev.vendor) == 0 && strcmp(entry->product, dev.product) == 0
            && entry->dhcp == ifcx.data.dhcp && strcmp(entry->dhcpHostname, ifcx.data.dhcpHostname) == 0
            && entry->pppoe == ifcx.data.pppoe && strcmp(entry->authName, ifcx.data.authName) == 0
            && strcmp(entry->authKey, ifcx.data.authKey) == 0) {
            addr = 0;
            Yn_ParseAddress((u8 *)entry->address, (u8 *)&addr);
            if (addr == *(u32 *)cfg->address) {
                addr = 0;
                Yn_ParseAddress((u8 *)entry->netmask, (u8 *)&addr);
                if (addr == *(u32 *)cfg->netmask) {
                    addr = 0;
                    Yn_ParseAddress((u8 *)entry->gateway, (u8 *)&addr);
                    if (addr == *(u32 *)cfg->gateway) {
                        addr = 0;
                        Yn_ParseAddress((u8 *)entry->dns1Address, (u8 *)&addr);
                        if (addr == *(u32 *)cfg->dns1) {
                            addr = 0;
                            Yn_ParseAddress((u8 *)entry->dns2Address, (u8 *)&addr);
                            if (addr == *(u32 *)cfg->dns2) {
                                return i;
                            }
                        }
                    }
                }
            }
        }
    }
    return -1;
}

/*
 * Returns 0 when the two proxy settings match (use, hosts and ports),
 * else -1.
 */
s32 Yn_CompareProxy(YnEntryKey *edited, YnEntryKey *inUse)
{
    if (edited->use != inUse->use) {
        return -1;
    }
    if (strcmp(edited->http, inUse->http) != 0) {
        return -1;
    }
    if (edited->httpPort != inUse->httpPort) {
        return -1;
    }
    if (strcmp(edited->https, inUse->https) != 0) {
        return -1;
    }
    if (edited->httpsPort != inUse->httpsPort) {
        return -1;
    }
    return 0;
}

/* Returns 1 when any of the address's four bytes is nonzero, else 0. */
s32 Yn_IsAddressSet(u8 *addr)
{
    s32 i;

    for (i = 0; i < 4; i++, addr++) {
        if (!*addr == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * Selects entry type 0, 1 or 2: points list, curData and countp at that
 * type's list, data and count.
 */
void YnNetcnf_SetType(YnNetcnf *n, s32 type)
{
    n->type = type;
    switch (type) {
    case 0:
        n->list = n->list0;
        n->curData = &n->cur[0];
        n->countp = &n->count[0];
        break;
    case 1:
        n->list = n->list1;
        n->curData = &n->cur[1];
        n->countp = &n->count[1];
        break;
    case 2:
        n->list = n->list2;
        n->curData = &n->cur[2];
        n->countp = &n->count[2];
        break;
    }
}

/*
 * Asks netcnfif how many entries of the selected type the file at path
 * has (waiting for the answer) and stores it in *countp. Returns it; 0 or
 * a negative error when there are none.
 */
s32 YnNetcnf_GetCount(YnNetcnf *n)
{
    sceNetcnfifGetCount(n->path, n->type);
    while (sceNetcnfifCheck() != 0) {
    }
    sceNetcnfifGetResult(&n->result);
    *n->countp = n->result;
    if (n->result <= 0) {
        return n->result;
    }
    return *n->countp;
}

/*
 * Reads the selected type's *countp entries into list (waiting for the
 * answer). Returns the count, or netcnfif's result when it is 0 or less.
 */
s32 YnNetcnf_GetList(YnNetcnf *n)
{
    if (*n->countp == 0) {
        return 0;
    }
    FlushCache(0);
    sceNetcnfifGetList(n->path, n->type, n->list, *n->countp);
    while (sceNetcnfifCheck() != 0) {
    }
    sceNetcnfifGetResult(&n->result);
    if (n->result <= 0) {
        return n->result;
    }
    return *n->countp;
}

/*
 * Loads every type 0 entry's full data into n->data: the count, the list,
 * then each entry by its usrName. Leaves type 0 selected.
 */
void YnNetcnf_LoadCombos(YnNetcnf *n)
{
    s32 i;

    memset(n->data, 0, sizeof(n->data));
    YnNetcnf_SetType(n, 0);
    YnNetcnf_GetCount(n);
    YnNetcnf_GetList(n);
    for (i = 0; i < n->count[0]; i++) {
        sceNetcnfifDataInit(&n->data[i]);
        FlushCache(0);
        sceNetcnfifLoadEntry(n->path, n->type, n->list[i].usrName, &n->data[i]);
        while (sceNetcnfifCheck() != 0) {
        }
    }
}

/*
 * Returns 1 when the file has no type 0 entries (or the count fails).
 * Keeps the selected type in savedType but leaves type 0 selected.
 */
s32 YnNetcnf_IsEmpty(YnNetcnf *n)
{
    n->savedType = n->type;
    YnNetcnf_SetType(n, 0);
    return YnNetcnf_GetCount(n) <= 0;
}

/*
 * Loads the first entry of the first non-empty type (0, 1, then 2) as a
 * test. Returns -1 when netcnfif answers -15 (seen: the list then shows
 * gYnOtherPs2Lines), else 0, also when the file has no entries.
 */
s32 YnNetcnf_CheckFile(YnNetcnf *n)
{
    n->savedType = n->type;
    YnNetcnf_SetType(n, 0);
    if (YnNetcnf_GetCount(n) > 0) {
        YnNetcnf_GetList(n);
        return YnNetcnf_LoadFirst(n);
    }
    YnNetcnf_SetType(n, 1);
    if (YnNetcnf_GetCount(n) > 0) {
        YnNetcnf_GetList(n);
        return YnNetcnf_LoadFirst(n);
    }
    YnNetcnf_SetType(n, 2);
    if (YnNetcnf_GetCount(n) > 0) {
        YnNetcnf_GetList(n);
        return YnNetcnf_LoadFirst(n);
    }
    return 0;
}

/*
 * Loads the first entry of the selected list into a local (retrying until
 * netcnfif takes the call), then reselects savedType. Returns -1 when the
 * result is -15, else 0.
 */
s32 YnNetcnf_LoadFirst(YnNetcnf *n)
{
    NetcnfData data;

    sceNetcnfifDataInit(&data);
    FlushCache(0);
    while (sceNetcnfifLoadEntry(n->path, n->type, n->list->usrName, &data) < 0) {
    }
    while (sceNetcnfifCheck() != 0) {
    }
    sceNetcnfifGetResult(&n->result);
    YnNetcnf_SetType(n, n->savedType);
    return -(n->result == -15);
}

void YnNetcnf_SetPath(YnNetcnf *n, s32 slot)
{
    sprintf(n->path, "mc%d:%s", slot, "/BWNETCNF/BWNETDCNF");
}

/* Parses a dotted address string into its four bytes (no checks). */
void Yn_ParseAddress(u8 *str, u8 *addr)
{
    u8 octet;

    octet = 0;
    for (; *str != 0; str++) {
        if (*str == '.') {
            *addr++ = octet;
            octet = 0;
        } else {
            octet *= 10;
            octet += *str - '0';
        }
    }
    *addr = octet;
}

void Yn_FormatAddress(char *str, u8 *addr)
{
    sprintf(str, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
}

/*
 * Stops the device in use, if any, then clears and refills the list of
 * attached network devices (NetDev_ClearList, NetDev_Refresh).
 */
void Yn_ResetNetDevices(void)
{
    if (gNetDevice != 0) {
        Net_StopDevice();
    }
    NetDev_ClearList();
    NetDev_Refresh();
}

/*
 * Returns the gNetDevs index of the attached device named vendor and
 * product; else -1 when it is a known model (NetDev_IsKnownName), -2 when
 * it is not.
 */
s32 Yn_FindDevice(char *vendor, char *product)
{
    s32 i;
    NetDevice *dev;

    for (i = 0, dev = gNetDevs; i < 35; i++, dev++) {
        if (dev->type != -1 && strcmp(dev->vendor, vendor) == 0 && strcmp(dev->product, product) == 0) {
            return i;
        }
    }
    return (NetDev_IsKnownName(vendor, product) != 0) ? -1 : -2;
}

/*
 * Returns 1 when the device named vendor and product is attached with
 * type 1, 2 when it is attached with another type, 0 when not attached.
 */
s32 Yn_GetDeviceKind(char *vendor, char *product)
{
    s32 i;
    NetDevice *dev;

    for (i = 0, dev = gNetDevs; i < 35; i++, dev++) {
        if (dev->type != -1 && strcmp(dev->vendor, vendor) == 0 && strcmp(dev->product, product) == 0) {
            if (gNetDevs[i].type == 1) {
                return 1;
            }
            return 2;
        }
    }
    return 0;
}

/*
 * Makes gNetDevs[i] the device in use and loads the drivers; returns 1.
 * With a device already in use returns 0 if it is this one, else -1;
 * -2 for a device not of type 1.
 */
s32 Yn_SelectDevice(s32 i)
{
    NetDevice *dev;

    dev = &gNetDevs[i];
    if (gNetDevice != NULL) {
        return -(gNetDevice != dev);
    }
    if (dev->type != 1) {
        return -2;
    }
    gNetDevice = dev;
    Net_LoadDrivers();
    Net_LoadDriverType1();
    return 1;
}

/*
 * Converts UTF-8 src to Shift-JIS in dst: ASCII is copied, hiragana and
 * katakana are mapped, any other three-byte character becomes one fixed
 * placeholder character.
 */
void Yn_Utf8ToSjis(char *dstp, char *srcp)
{
    u8 *dst = (u8 *)dstp;
    u8 *src = (u8 *)srcp;
    u16 code;

    code = *src;
    while (code != 0) {
        if (code >= 128) {
            code = (src[1] << 8) + *(src += 2);
            src++;
            if (code >= 0x8181 && code < 0x81C0) {
                code += 286;
            } else if (code >= 0x8280 && code < 0x8294) {
                code += 94;
            } else if (code >= 0x82A1 && code < 0x82C0) {
                code += 159;
            } else if (code >= 0x8380 && code < 0x83A0) {
                code -= 33;
            } else if (code >= 0x83A0 && code < 0x83B7) {
                code -= 32;
            } else {
                code = 33115;
            }
            dst[0] = (code >> 8) & 0xFF;
            dst[1] = code & 0xFF;
            dst += 2;
        } else {
            *dst = code;
            src++;
            dst++;
        }
        code = *src;
    }
    *dst = 0;
}

/*
 * Converts Shift-JIS src to UTF-8 in dst: ASCII is copied, kana become
 * three-byte sequences, other two-byte characters one fixed placeholder;
 * stops before the output passes 255 bytes.
 */
void Yn_SjisToUtf8(char *dstp, char *srcp)
{
    u8 *dst = (u8 *)dstp;
    u8 *src = (u8 *)srcp;
    s32 n;
    u16 code;

    n = 0;
    code = *src;
    while (code != 0) {
        if (code >= 128) {
            if (n >= 253) {
                break;
            }
            code = ((code & 0xFF) << 8) + src[1];
            src += 2;
            if (code >= 0x829F && code < 0x82DE) {
                code -= 286;
            } else if (code >= 0x82DE && code < 0x82F2) {
                code -= 94;
            } else if (code >= 0x8340 && code < 0x835F) {
                code -= 159;
            } else if (code >= 0x835F && code < 0x837F) {
                code += 33;
            } else if (code >= 0x8380 && code < 0x8397) {
                code += 32;
            } else {
                code = 33724;
            }
            dst[0] = 227;
            dst[1] = (code >> 8) & 0xFF;
            n += 3;
            *(dst += 2) = code & 0xFF;
            dst++;
        } else {
            if (n >= 255) {
                break;
            }
            *dst = code;
            src++;
            dst++;
            n++;
        }
        code = *src;
    }
    *dst = 0;
}

/*
 * Stops IOP module id, then unloads it, retrying each call until the IOP
 * accepts it.
 */
void Yn_UnloadModule(s32 id)
{
    s32 result;
    s32 ret;

    while ((ret = sceSifStopModule(id, 0, 0, &result)) < 0) {
        YN_WAIT();
    }
    if (0 <= ret) {
        while (sceSifUnloadModule(id) < 0) {
            YN_WAIT();
        }
    }
}

/*
 * Loads the IOP module at path with argc and argv: sceSifLoadModule, or
 * sceSifLoadStartModule (given buf) when buf is set. tries 0 retries
 * until it works, else gives up after tries attempts.
 */
void Yn_LoadModule(char *path, s32 argc, char *argv, void *buf, s32 tries)
{
    s32 i;

    if (buf == NULL) {
        if (tries == 0) {
            while (sceSifLoadModule(path, argc, argv) < 0) {
                YN_WAIT();
            }
        } else {
            for (i = 0; i < tries; i++) {
                if (0 <= sceSifLoadModule(path, argc, argv)) {
                    i = 0;
                    break;
                }
                YN_WAIT7();
            }
        }
    } else {
        if (tries == 0) {
            while (sceSifLoadStartModule(path, argc, argv, buf) < 0) {
                YN_WAIT();
            }
        } else {
            for (i = 0; i < tries; i++) {
                if (0 <= sceSifLoadStartModule(path, argc, argv, buf)) {
                    i = 0;
                    break;
                }
                YN_WAIT7();
            }
        }
    }
}


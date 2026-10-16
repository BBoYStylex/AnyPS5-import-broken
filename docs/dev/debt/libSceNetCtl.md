# libSceNetCtl technical debt

### Unknown function info

- [sceNetCtlGetInfo](../../../core/libs/prx/libSceNetCtl/Export.cpp) (libSceNetCtl) - answers from the host interface that holds an IPv4 address, preferring the one that carries the default route: its address, netmask, gateway, MTU and first two IPv4 DNS servers. The device is reported as wired, the link as connected, the IP configuration as DHCP and the HTTP proxy as off, whatever the host uses, and the Ethernet address is the one `sceNetGetMacAddress` reports. A gateway or DNS server the interface does not have is reported as `0.0.0.0`; what a console reports then is unknown. The Wi-Fi codes (5-10), the DHCP host name (12), the PPPoE user name (13) and the HTTP proxy server and port (20, 21) throw
- [sceNetCtlGetIfStat](../../../core/libs/prx/libSceNetCtl/Export.cpp) (libSceNetCtl) - signature and 56-byte layout come from PPSA09030's call site and SharpProspero's `SceNetCtlIfStat`; the device kind is always 0, the byte counters are the host's totals over its up non-loopback hardware interfaces (not "since cleared"), and the not-connected result is assumed to match `sceNetCtlGetState`

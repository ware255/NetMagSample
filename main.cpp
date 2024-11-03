#include <cstdio>
#include <windows.h>
#include <wlanapi.h>
#include <iphlpapi.h>

char* WChar2Char(const wchar_t* wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (size_needed <= 0)
        return nullptr;

    char* cstr = new char[size_needed];
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, cstr, size_needed, NULL, NULL);
    
    return cstr;
}

int main() {
    HANDLE hClient = NULL;
    DWORD dwMaxClient = 2;
    DWORD dwCurVersion = 0;
    PIP_ADAPTER_INFO AdapterInfo = NULL;
    DWORD dwBufLen = sizeof(IP_ADAPTER_INFO);
    char mac_addr[18] = {};

    if (WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient) != ERROR_SUCCESS) {
        printf("WlanOpenHandle failed.\n");
        return 1;
    }

    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
    if (WlanEnumInterfaces(hClient, NULL, &pIfList) != ERROR_SUCCESS) {
        printf("WlanEnumInterfaces failed.\n");
        WlanCloseHandle(hClient, NULL);
        return 1;
    }

    GUID interfaceGuid = pIfList->InterfaceInfo[0].InterfaceGuid;

    PWLAN_AVAILABLE_NETWORK_LIST pNetworkList = NULL;
    if (WlanGetAvailableNetworkList(hClient, &interfaceGuid, 0, NULL, &pNetworkList) != ERROR_SUCCESS) {
        printf("WlanGetAvailableNetworkList failed.\n");
        WlanFreeMemory(pIfList);
        WlanCloseHandle(hClient, NULL);
        return 1;
    }

    char* adapter_interface = NULL;
    for (int i = 0; i < (int)pIfList->dwNumberOfItems; i++) {
        PWLAN_INTERFACE_INFO pIfInfo = &pIfList->InterfaceInfo[i];
        PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;
        DWORD dwMaxSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
        WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;

        if (WlanQueryInterface(hClient, &pIfInfo->InterfaceGuid, wlan_intf_opcode_current_connection, NULL, &dwMaxSize, (PVOID*)&pConnectInfo, &opCode) == ERROR_SUCCESS) {
            if (pConnectInfo->isState == wlan_interface_state_connected) {
                // SSIDを表示
                adapter_interface = WChar2Char(pIfInfo->strInterfaceDescription);
                printf("Adapter interface: %s\n", adapter_interface);
                printf("Connected SSID: %s\n", pConnectInfo->wlanAssociationAttributes.dot11Ssid.ucSSID);
                printf("Chiper: ");
                switch (pConnectInfo->wlanSecurityAttributes.dot11AuthAlgorithm) {
                case DOT11_AUTH_ALGO_80211_OPEN:
                    printf("no\n");
                    break;
                case DOT11_AUTH_ALGO_80211_SHARED_KEY:
                    printf("WEP\n");
                    break;
                case DOT11_AUTH_ALGO_WPA:
                    printf("WPA\n");
                    break;
                case DOT11_AUTH_ALGO_WPA_PSK:
                    printf("WPA_PSK\n");
                    break;
                case DOT11_AUTH_ALGO_WPA_NONE:
                    printf("No Authentication\n");
                    break;
                case DOT11_AUTH_ALGO_RSNA:
                    printf("RSNA (WPA2)\n");
                    break;
                case DOT11_AUTH_ALGO_RSNA_PSK:
                    printf("RSNA-PSK (WPA2-PSK)\n");
                    break;
                default:
                    printf("WPA3 ?\n");
                }
            }
            if (pConnectInfo) {
                WlanFreeMemory(pConnectInfo);
            }
            break;
        }
    }

    if (pIfList)
        WlanFreeMemory(pIfList);

    WlanCloseHandle(hClient, NULL);

    AdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
    if (AdapterInfo == NULL) {
        printf("Error allocating memory needed to call GetAdaptersinfo\n");
        return 1;
    }

    if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(AdapterInfo);
        AdapterInfo = (IP_ADAPTER_INFO *)malloc(dwBufLen);
        if (AdapterInfo == NULL) {
            printf("Error allocating memory needed to call GetAdaptersinfo\n");
            return 1;
        }
    }

    if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
        while (pAdapterInfo) {
            if (!strcmp(pAdapterInfo->Description, adapter_interface)) {
                sprintf(mac_addr, "%02X:%02X:%02X:%02X:%02X:%02X",
                    pAdapterInfo->Address[0], pAdapterInfo->Address[1],
                    pAdapterInfo->Address[2], pAdapterInfo->Address[3],
                    pAdapterInfo->Address[4], pAdapterInfo->Address[5]);

                PIP_ADDR_STRING pIpAddr = &pAdapterInfo->IpAddressList;
                printf("MAC Address: %s\n", mac_addr);
                printf("IP Address: %s\n", pIpAddr->IpAddress.String);
                break;
            }
            pAdapterInfo = pAdapterInfo->Next;
        }
    }

    if (AdapterInfo)
        free(AdapterInfo);
    if (adapter_interface)
        free(adapter_interface);

    return 0;
}

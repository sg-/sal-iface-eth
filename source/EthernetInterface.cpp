/* EthernetInterface.cpp */
/* Copyright (C) 2012 mbed.org, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "EthernetInterface.h"

#include "lwip/inet.h"
#include "lwip/netif.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "eth_arch.h"
#include "lwip/tcpip.h"
#include "lwip/init.h"

#include "mbed.h"
#include <stdint.h>

/* TCP/IP and Network Interface Initialisation */
static struct netif netif;

static char mac_addr[19];
static char ip_addr[17] = "\0";
static char gateway[17] = "\0";
static char networkmask[17] = "\0";
static bool use_dhcp = false;

static volatile uint8_t link_up;
static volatile uint8_t if_up;
extern volatile uint8_t allow_net_callbacks;

static void netif_link_callback(struct netif *netif) {
    link_up = 1;
}

static void netif_status_callback(struct netif *netif) {
    if (netif_is_up(netif)) {
        strcpy(ip_addr, inet_ntoa(netif->ip_addr));
        strcpy(gateway, inet_ntoa(netif->gw));
        strcpy(networkmask, inet_ntoa(netif->netmask));
        if_up = 1;
    }
}

static void init_netif(ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw) {
    lwip_init();

    memset((void*) &netif, 0, sizeof(netif));
    netif_add(&netif, ipaddr, netmask, gw, NULL, eth_arch_enetif_init, ethernet_input);
    netif_set_default(&netif);

    netif_set_link_callback  (&netif, netif_link_callback);
    netif_set_status_callback(&netif, netif_status_callback);

    allow_net_callbacks = 1;
}

static void set_mac_address(void) {
#if (MBED_MAC_ADDRESS_SUM != MBED_MAC_ADDR_INTERFACE)
    snprintf(mac_addr, 19, "%02x:%02x:%02x:%02x:%02x:%02x", MBED_MAC_ADDR_0, MBED_MAC_ADDR_1, MBED_MAC_ADDR_2,
             MBED_MAC_ADDR_3, MBED_MAC_ADDR_4, MBED_MAC_ADDR_5);
#else
    char mac[6];
    mbed_mac_address(mac);
    snprintf(mac_addr, 19, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#endif
}

int EthernetInterface::init() {
    use_dhcp = true;
    set_mac_address();
    init_netif(NULL, NULL, NULL);
    return 0;
}

int EthernetInterface::init(const char* ip, const char* mask, const char* gateway) {
    use_dhcp = false;

    set_mac_address();
    strcpy(ip_addr, ip);

    ip_addr_t ip_n, mask_n, gateway_n;
    inet_aton(ip, &ip_n);
    inet_aton(mask, &mask_n);
    inet_aton(gateway, &gateway_n);
    init_netif(&ip_n, &mask_n, &gateway_n);

    return 0;
}

int EthernetInterface::connect(unsigned int timeout_ms) {
    eth_arch_enable_interrupts();

    // TODO: implement by (busy?)waiting for IP address
    int inited = 0;
    if (use_dhcp) {
        dhcp_start(&netif);

        // Wait for an IP Address
        // -1: error, 0: timeout
        while (if_up == 0);
    } else {
        netif_set_up(&netif);
        while (link_up == 0);
    }

    return inited;
}

int EthernetInterface::disconnect() {
    if (use_dhcp) {
        dhcp_release(&netif);
        dhcp_stop(&netif);
    } else {
        netif_set_down(&netif);
    }

    eth_arch_disable_interrupts();
    if_up = link_up = 0;
    return 0;
}

char* EthernetInterface::getMACAddress() {
    return mac_addr;
}

char* EthernetInterface::getIPAddress() {
    return ip_addr;
}

char* EthernetInterface::getGateway() {
    return gateway;
}

char* EthernetInterface::getNetworkMask() {
    return networkmask;
}


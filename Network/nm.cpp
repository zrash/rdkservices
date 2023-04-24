#if 0
typedef enum {
    NM_DEVICE_STATE_UNKNOWN      = 0,
    NM_DEVICE_STATE_UNMANAGED    = 10,
    NM_DEVICE_STATE_UNAVAILABLE  = 20,
    NM_DEVICE_STATE_DISCONNECTED = 30,
    NM_DEVICE_STATE_PREPARE      = 40,
    NM_DEVICE_STATE_CONFIG       = 50,
    NM_DEVICE_STATE_NEED_AUTH    = 60,
    NM_DEVICE_STATE_IP_CONFIG    = 70,
    NM_DEVICE_STATE_IP_CHECK     = 80,
    NM_DEVICE_STATE_SECONDARIES  = 90,
    NM_DEVICE_STATE_ACTIVATED    = 100,
    NM_DEVICE_STATE_DEACTIVATING = 110,
    NM_DEVICE_STATE_FAILED       = 120,
} NMDeviceState;


typedef enum {
    NM_CONNECTIVITY_UNKNOWN = 0,
    NM_CONNECTIVITY_NONE    = 1,
    NM_CONNECTIVITY_PORTAL  = 2,
    NM_CONNECTIVITY_LIMITED = 3,
    NM_CONNECTIVITY_FULL    = 4,
} NMConnectivityState;

nm_connectivity_to_string

typedef enum {
    NM_ACTIVE_CONNECTION_STATE_UNKNOWN      = 0,
    NM_ACTIVE_CONNECTION_STATE_ACTIVATING   = 1,
    NM_ACTIVE_CONNECTION_STATE_ACTIVATED    = 2,
    NM_ACTIVE_CONNECTION_STATE_DEACTIVATING = 3,
    NM_ACTIVE_CONNECTION_STATE_DEACTIVATED  = 4,
} NMActiveConnectionState;

typedef enum {
    NM_ACTIVE_CONNECTION_STATE_REASON_UNKNOWN               = 0,
    NM_ACTIVE_CONNECTION_STATE_REASON_NONE                  = 1,
    NM_ACTIVE_CONNECTION_STATE_REASON_USER_DISCONNECTED     = 2,
    NM_ACTIVE_CONNECTION_STATE_REASON_DEVICE_DISCONNECTED   = 3,
    NM_ACTIVE_CONNECTION_STATE_REASON_SERVICE_STOPPED       = 4,
    NM_ACTIVE_CONNECTION_STATE_REASON_IP_CONFIG_INVALID     = 5,
    NM_ACTIVE_CONNECTION_STATE_REASON_CONNECT_TIMEOUT       = 6,
    NM_ACTIVE_CONNECTION_STATE_REASON_SERVICE_START_TIMEOUT = 7,
    NM_ACTIVE_CONNECTION_STATE_REASON_SERVICE_START_FAILED  = 8,
    NM_ACTIVE_CONNECTION_STATE_REASON_NO_SECRETS            = 9,
    NM_ACTIVE_CONNECTION_STATE_REASON_LOGIN_FAILED          = 10,
    NM_ACTIVE_CONNECTION_STATE_REASON_CONNECTION_REMOVED    = 11,
    NM_ACTIVE_CONNECTION_STATE_REASON_DEPENDENCY_FAILED     = 12,
    NM_ACTIVE_CONNECTION_STATE_REASON_DEVICE_REALIZE_FAILED = 13,
    NM_ACTIVE_CONNECTION_STATE_REASON_DEVICE_REMOVED        = 14,
} NMActiveConnectionStateReason;

typedef enum {
    NM_DEVICE_STATE_UNKNOWN      = 0,
    NM_DEVICE_STATE_UNMANAGED    = 10,
    NM_DEVICE_STATE_UNAVAILABLE  = 20,
    NM_DEVICE_STATE_DISCONNECTED = 30,
    NM_DEVICE_STATE_PREPARE      = 40,
    NM_DEVICE_STATE_CONFIG       = 50,
    NM_DEVICE_STATE_NEED_AUTH    = 60,
    NM_DEVICE_STATE_IP_CONFIG    = 70,
    NM_DEVICE_STATE_IP_CHECK     = 80,
    NM_DEVICE_STATE_SECONDARIES  = 90,
    NM_DEVICE_STATE_ACTIVATED    = 100,
    NM_DEVICE_STATE_DEACTIVATING = 110,
    NM_DEVICE_STATE_FAILED       = 120,
} NMDeviceState;


#endif

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <NetworkManager.h>
#include <stdbool.h>

#define PREFIX_TO_NETMASK(prefix_len) ({ \
    static char netmask_str[16]; \
    uint32_t netmask = 0xffffffff << (32 - (prefix_len)); \
    snprintf(netmask_str, 16, "%u.%u.%u.%u", \
             (netmask >> 24) & 0xff, \
             (netmask >> 16) & 0xff, \
             (netmask >> 8) & 0xff, \
             netmask & 0xff); \
    netmask_str; \
})

NMClient *client = NULL;

int isInterfaceEnabled(char * interface); 
int getinterfaces(void);
int isConnectedToInternet(void);
int getDefaultInterface(void);
int getIpSettings(void);
char* getQuirks(void);
char* getNamedEndpoints(void);
int getStbIp(void);
int getStbIpFamily(char * f);
int setDefaultInterface(char* interface, bool enable, bool persist);

int setDefaultInterface(char* interface, bool enable, bool persist)
{
return 0;


}
int getStbIp(void)
{
    GError *error = NULL;
    NMActiveConnection *active_conn = NULL;
    NMIPConfig *ipconfig = NULL;

    active_conn = nm_client_get_primary_connection(client);
    if (active_conn == NULL) {
        fprintf(stderr, "Error getting primary connection: %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    ipconfig = nm_active_connection_get_ip4_config(active_conn);
    if (ipconfig != NULL) {
        const GPtrArray *p;
        int              i;
        p = nm_ip_config_get_addresses(ipconfig);
        for (i = 0; i < p->len; i++) {
            NMIPAddress *a = p->pdata[i];
            g_print("\tinet4 %s/%d\n", nm_ip_address_get_address(a), nm_ip_address_get_prefix(a));
        }
    }

    return 0;
}

int getStbIpFamily(char*f)
{
    GError *error = NULL;
    NMActiveConnection *active_conn = NULL;
    NMIPConfig *ipconfig = NULL;

    active_conn = nm_client_get_primary_connection(client);
    if (active_conn == NULL) {
        fprintf(stderr, "Error getting primary connection: %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    if(strcmp(f,"AF_INET")==0){
        printf("Requested for IPV4 family \n");
    }else if(strcmp(f,"AF_INET6")==0){
        printf("Requested for IPV6 family \n");
    }
    ipconfig = nm_active_connection_get_ip4_config(active_conn);
    if (ipconfig != NULL) {
        const GPtrArray *p;
        int              i;
        p = nm_ip_config_get_addresses(ipconfig);
        for (i = 0; i < p->len; i++) {
            NMIPAddress *a = p->pdata[i];
            g_print("\tinet4 %s/%d\n", nm_ip_address_get_address(a), nm_ip_address_get_prefix(a));
        }
    }
    ipconfig = nm_active_connection_get_ip6_config(active_conn);
    if (ipconfig != NULL) {
        const GPtrArray *p;
        int              i;
        p = nm_ip_config_get_addresses(ipconfig);
        for (i = 0; i < p->len; i++) {
            NMIPAddress *a = p->pdata[i];
            g_print("\tinet4 %s/%d\n", nm_ip_address_get_address(a), nm_ip_address_get_prefix(a));
        }
    }

    return 0;
}
int isInterfaceEnabled(char * interface) 
{
    NMDevice *device ;
    NMDeviceState state;

    const char *iface_name = NULL; // replace with your interface name

    if (!(strcmp (interface, "ETHERNET") == 0 || strcmp (interface, "WIFI") == 0))
    {
        printf("failed due to invalid interface [%s]", interface);
        return -1;
    }
    if(strcmp (interface, "ETHERNET") == 0){
        iface_name = "eno1";  /*TODO get value using getenvOrDefault */
        device = nm_client_get_device_by_iface(client, iface_name);
        if (device == NULL) {
            printf("ethernet interface not available\n");
        } else {
            printf("ethernet interface available\n");
        }
        // check the state of the device
        state = nm_device_get_state(device);
        if (state == NM_DEVICE_STATE_UNMANAGED || 
            state == NM_DEVICE_STATE_UNKNOWN   ||
            state == NM_DEVICE_STATE_UNAVAILABLE ) {
            printf("Interface %s is disabled\n", iface_name);
        }
        else{
            printf("Interface %s is enabled\n", iface_name);
        }
    }
    else if(strcmp (interface, "WIFI") == 0){
        iface_name = "wlxc4e90a09837c";  /*TODO get value using getenvOrDefault */
        device  = nm_client_get_device_by_iface(client,iface_name );
        if (device == NULL) {
            printf("WiFi interface not available\n");
        } else {
            printf("WiFi interface available\n");
        }
        // check the state of the device
        state = nm_device_get_state(device);
        if (state == NM_DEVICE_STATE_UNMANAGED || 
            state == NM_DEVICE_STATE_UNKNOWN   ||
            state == NM_DEVICE_STATE_UNAVAILABLE ) {
            printf("Interface %s is disabled\n", iface_name);
        }
        else{
            printf("Interface %s is enabled\n", iface_name);
        }

    }

    printf(" connectivity = internet %s state = %d \n", (nm_client_get_connectivity(client) >2)?"Connected":"Disconnected", state);


    return 0; 
}

int getinterfaces()
{
    const GPtrArray *devices = nm_client_get_devices(client);
    for (guint i = 0; i < devices->len; i++) {
        NMDevice *device = g_ptr_array_index(devices, i);
        const char *interface_name = nm_device_get_iface(device);
        const char *hw_address = nm_device_get_hw_address(device);
        NMDeviceState state = nm_device_get_state(device);
        const char *state_str = (state > NM_DEVICE_STATE_UNAVAILABLE) ? "interface:enabled" : "interface:disabled";
        NMConnectivityState connectivity  = nm_device_get_connectivity (device, AF_INET);
        const char *conn_str = (connectivity > NM_CONNECTIVITY_LIMITED) ? "connected:true" : "connected:false";

        printf("Interface %u: %s (%s, %s, %s )\n", i, interface_name, hw_address, state_str, conn_str);
    }
    return 0;
}

int isConnectedToInternet()
{

    printf(" connectivity = internet %s\n", (nm_client_get_connectivity(client) >2)?"Connected":"Disconnected");
    return 0;

}

int getDefaultInterface()
{
    GError *error = NULL;
    NMActiveConnection *active_conn = NULL;
    NMRemoteConnection *newconn = NULL;
    NMSettingConnection *s_con;
    const char *interface_name = NULL;

    active_conn = nm_client_get_primary_connection(client);
    if (active_conn == NULL) {
        fprintf(stderr, "Error getting primary connection: %s\n", error->message);
        g_error_free(error);
        return 1;
    }
    newconn = nm_active_connection_get_connection(active_conn);
    s_con = nm_connection_get_setting_connection(NM_CONNECTION(newconn));

    interface_name = nm_connection_get_interface_name(NM_CONNECTION(newconn));
    printf("Interface name: %s\n", interface_name);
    if (g_strcmp0(nm_setting_connection_get_connection_type(s_con), NM_SETTING_WIRELESS_SETTING_NAME) == 0) {
        printf("default interface = %s(%s)\n", "WIFI", NM_SETTING_WIRELESS_SETTING_NAME);
    }
    else if (g_strcmp0(nm_setting_connection_get_connection_type(s_con), NM_SETTING_WIRED_SETTING_NAME) == 0) {
        printf("default interface = %s(%s)\n", "ETHERNET", NM_SETTING_WIRED_SETTING_NAME);
    }

    return 0;
}

int getIpSettings()
{

    char* f = "AF_INET";
    GError *error = NULL;
    NMActiveConnection *active_conn = NULL;
    NMIPConfig *ip4_config = NULL;
    NMIPConfig *ip6_config = NULL;
    const char *gateway = NULL;
	char **dns_arr = NULL;
    NMDhcpConfig *dhcp4_config = NULL;
    NMDhcpConfig *dhcp6_config = NULL;
    GHashTable * ght ;

    active_conn = nm_client_get_primary_connection(client);
    if (active_conn == NULL) {
        fprintf(stderr, "Error getting primary connection: %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    if(strcmp(f,"AF_INET")==0){
        printf("Requested for IPV4 family \n");
    }else if(strcmp(f,"AF_INET6")==0){
        printf("Requested for IPV6 family \n");
    }
    printf("********IPv4 *********\n");
    ip4_config = nm_active_connection_get_ip4_config(active_conn);
    if (ip4_config != NULL) {
        const GPtrArray *p;
        int              i;
        p = nm_ip_config_get_addresses(ip4_config);
        for (i = 0; i < p->len; i++) {
            NMIPAddress *a = p->pdata[i];
            g_print("\tinet4 %s/%d, %s \n", nm_ip_address_get_address(a), nm_ip_address_get_prefix(a), PREFIX_TO_NETMASK( nm_ip_address_get_prefix(a)));
        }
        gateway = nm_ip_config_get_gateway(ip4_config);
        printf("Gateway = %s \n", gateway);
    }
    dns_arr =   (char **)nm_ip_config_get_nameservers(ip4_config);
    if((*(&dns_arr[0]))!=NULL )
    printf("Primary DNS: %s\n", *(&dns_arr[0]));
    if((*(&dns_arr[1]))!=NULL )
        printf("Secondary DNS: %s\n", *(&dns_arr[1]));
    
    dhcp4_config = nm_active_connection_get_dhcp4_config(active_conn);
    ght = nm_dhcp_config_get_options(dhcp4_config);
    printf("dhcp 4= %s , \n",(char *)g_hash_table_lookup(ght,"dhcp_server_identifier") );

    printf("********IPv6 *********\n");
    ip6_config = nm_active_connection_get_ip6_config(active_conn);
    if (ip6_config != NULL) {
        const GPtrArray *p;
        int              i;
        p = nm_ip_config_get_addresses(ip6_config);
        for (i = 0; i < p->len; i++) {
            NMIPAddress *a = p->pdata[i];
            g_print("\tinet6 %s/%d\n", nm_ip_address_get_address(a), nm_ip_address_get_prefix(a));
        }
        gateway = nm_ip_config_get_gateway(ip6_config);
        printf("Gateway = %s \n", gateway);

    dns_arr =   (char **)nm_ip_config_get_nameservers(ip6_config);
    if((*(&dns_arr[0]))!=NULL )
    printf("6 primary DNS: %s\n", *(&dns_arr[0]));
    if((*(&dns_arr[1]))!=NULL )
    printf("6 Secondary DNS: %s\n", *(&dns_arr[1]));

    dhcp6_config = nm_active_connection_get_dhcp6_config(active_conn);
    ght = nm_dhcp_config_get_options(dhcp6_config);
    printf("dhcp 6= %s , \n",(char *)g_hash_table_lookup(ght,"dhcp_server_identifier"));

    }

    return 0;

}

char* getNamedEndpoints()
{
    return "CMTS";
#if 0
    uint32_t Network::getNamedEndpoints(const JsonObject& parameters, JsonObject& response)
    {
        JsonArray namedEndpoints;
        namedEndpoints.Add("CMTS");

        response["endpoints"] = namedEndpoints;
        returnResponse(true)
    }
#endif
}

char* getQuirks(void)
{
    return "RDK-20093";
#if 0
uint32_t Network::getQuirks(const JsonObject& parameters, JsonObject& response)
{
JsonArray array;
array.Add("RDK-20093");
response["quirks"] = array;
returnResponse(true)
}
#endif
}


int main(int argc, char *argv[]) {
    GError *error = NULL;
    NMDevice *device = NULL;
    const char *iface_name = "eno1"; // replace with your interface name
    NMDeviceState state;


    // initialize the NMClient object
    client = nm_client_new(NULL, &error);
    if (client == NULL) {
        fprintf(stderr, "Error initializing NMClient: %s\n", error->message);
        g_error_free(error);
        return EXIT_FAILURE;
    }

    // get the device object for the specified interface name
    device = nm_client_get_device_by_iface(client, iface_name);
    if (device == NULL) {
        fprintf(stderr, "Error getting device for interface %s: %s\n", iface_name, strerror(errno));
        return EXIT_FAILURE;
    }

    // check the state of the device
    state = nm_device_get_state(device);
    if (state == NM_DEVICE_STATE_ACTIVATED) {
        printf("Interface %s is enabled and active.\n", iface_name);
    } else if (state == NM_DEVICE_STATE_UNMANAGED) {
        printf("Interface %s is unmanaged.\n", iface_name);
    } else if (state == NM_DEVICE_STATE_DISCONNECTED) {
        printf("Interface %s is enabled but not active.\n", iface_name);
    } else {
        printf("Interface %s is in an unknown state.\n", iface_name);
    }

    // get the device object for the specified interface name
    device = nm_client_get_device_by_iface(client, iface_name);
    if (device == NULL) {
        fprintf(stderr, "Error getting device for interface %s: %s\n", iface_name, strerror(errno));
        return EXIT_FAILURE;
    }

    // check the state of the device
    state = nm_device_get_state(device);
    if (state == NM_DEVICE_STATE_ACTIVATED) {
        printf("Interface %s is enabled and active.\n", iface_name);
    } else if (state == NM_DEVICE_STATE_UNMANAGED) {
        printf("Interface %s is unmanaged.\n", iface_name);
    } else if (state == NM_DEVICE_STATE_DISCONNECTED) {
        printf("Interface %s is enabled but not active.\n", iface_name);
    } else {
        printf("Interface %s is in an unknown state.\n", iface_name);
    }


    switch(1) {
	case 1:
        printf("\n\n\nisInterfaceEnabled\n");
		isInterfaceEnabled( "ETHERNET");
		isInterfaceEnabled( "WIFI");

        printf("\n\n\n  getinterfaces\n");
        getinterfaces();

        printf("\n\n\n isConnectedToInternet\n");
        isConnectedToInternet();

        printf("\n\n\n getDefaultInterface\n");
        getDefaultInterface();

        printf("\n\n\n getQuirks\n");
        printf(" getQuirks = %s\n", getQuirks());

        printf("\n\n\n getNamedEndpoints\n");
        printf(" getNamedEndpoints = %s\n", getNamedEndpoints());


        printf("\n\n\n getStbIp\n");
        getStbIp();
        getStbIpFamily("AF_INET");
        getStbIpFamily("AF_INET6");

        printf("\n\n\n getIpSettings \n");
        getIpSettings();
        printf("\n\n\n setDefaultInterface\n");
        setDefaultInterface("WIFI", TRUE, TRUE);
        setDefaultInterface("WIFI", FALSE, TRUE);
        setDefaultInterface("WIFI", TRUE, FALSE);
        setDefaultInterface("WIFI", FALSE, FALSE);
        setDefaultInterface("ETHERNET", TRUE, TRUE);
        setDefaultInterface("ETHERNET", FALSE, TRUE);
        setDefaultInterface("ETHERNET", TRUE, FALSE);
        setDefaultInterface("ETHERNET", FALSE, FALSE);
    }


   // clean up
    //g_object_unref(device);
    g_object_unref(client);

    return EXIT_SUCCESS;
}


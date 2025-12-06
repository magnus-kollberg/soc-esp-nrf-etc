#ifndef __network_h__
#define __network_h__

#ifndef CONFIG_SHOW_PASSWORD
#define CONFIG_SHOW_PASSWORD 0
#endif

#define CONFIG_NETWORK_PROVISONING_KOLLBERG 1
#define CONFIG_NETWORK_PROVISONING_SOFTAP   2
#define CONFIG_NETWORK_PROVISONING_BLE      3

#ifndef CONFIG_NETWORK_PROVISONING
#define CONFIG_NETWORK_PROVISONING CONFIG_NETWORK_PROVISONING_KOLLBERG
#endif

typedef struct
{
    String staName;
    String staPassprase;
    String apName;
    String apPassprase;
} networkSetup_t;

void networkSetup(networkSetup_t *p_network_setup);

void networkLoop(void);

#endif
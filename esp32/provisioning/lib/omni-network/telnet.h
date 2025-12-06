#ifndef __telnet_h__
#define __telnet_h__

struct TelnetConfig
{
    bool enabled;
};

int telnetSetup(void);
TelnetConfig telnetConfigLoad(void);
int telnetConfigSave(const TelnetConfig *p_config);

#endif

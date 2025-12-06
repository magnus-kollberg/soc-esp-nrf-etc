#include "tools.h"
#include <WiFi.h>

WiFiServer telnetServer(23);
WiFiClient telnetClient;

void telnetSetup()
{
    telnetServer.begin();
    telnetServer.setNoDelay(true);
    myPrintf("Telnet server started.");
}

void telnetLoop()
{
    // Check for new Telnet client connection
    if (telnetServer.hasClient())
    {
        if (telnetClient && telnetClient.connected())
        {
            myPrintf("Client already connected. Rejecting new connection.");
            telnetServer.available().stop();
        }
        else
        {
            myPrintf("New Telnet client connected.");
            telnetClient = telnetServer.available();
            telnetClient.println("Welcome to ESP32 Telnet Server!");
        }
    }

    // Handle Telnet client input
    if (telnetClient && telnetClient.connected() && telnetClient.available())
    {
        String clientInput = telnetClient.readStringUntil('\n');
        myPrintf("Client says: ");
        myPrintf(clientInput.c_str());

        // Echo back the input
        telnetClient.print("You said: ");
        telnetClient.println(clientInput);
    }

    // Monitor Telnet disconnection
    if (telnetClient && !telnetClient.connected())
    {
        myPrintf("Telnet client disconnected.");
        telnetClient.stop();
    }
}
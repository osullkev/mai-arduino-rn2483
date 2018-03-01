#ifndef networkDetails_h
#define networkDetails_h

#include "Arduino.h"

class networkDetails
{
    public:
        networkDetails(String name);
        String getAppEUI();
        String getAppKey();
    private:
        String _networkName;
        String _appEUI = ; // Insert APP EUI; 
        String _appKey = ; // Insert APP KEY;
};


#endif

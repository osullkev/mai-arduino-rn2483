#include "Arduino.h"
#include "networkDetails.h"

networkDetails::networkDetails(String name)
{
    _networkName = name;
}


String networkDetails::getAppEUI()
{
    return _appEUI;
}

String networkDetails::getAppKey()
{
    return _appKey;
}

#include <Definitions.h>
#include <string>
#include <iostream>
#include <sstream>
#include <list>

#ifndef MMC_SUCCESS
#define MMC_SUCCESS 0
#endif

#ifndef MMC_FAILED
#define MMC_FAILED 1
#endif

using namespace std;
HANDLE g_keyHandle = 0;
void* g_pKeyHandle = 0;
WORD g_usNodeId = 1;
string g_deviceName;
string g_protocolStackName;
string g_interfaceName;
string g_portName;
DWORD g_baudrate = 0;
const string g_programName = "Maxon Test Program";

void SetDefaultParameters();
int OpenDevice(DWORD* p_pErrorCode);
int CloseDevice(DWORD* p_pErrorCode);
int PrepareDemo(DWORD * p_pErrorCode);
int DemoProfileVelocityMode(HANDLE p_DeviceHandle, WORD p_usNodeId, DWORD & p_rlErrorCode);
int DemoProfilePositionMode(HANDLE p_DeviceHandle, WORD p_usNodeId, DWORD & p_rlErrorCode);
int Demo(DWORD * p_pErrorCode);
int PrintAvailableInterfaces();
int PrintAvailablePorts(char* p_pInterfaceNameSel);
int PrintDeviceVersion();
void  PrintSettings();
void LogError(string functionName, int p_lResult, DWORD p_ulErrorCode);

int main(int argc, char** argv) {
    int lResult = MMC_FAILED;
    DWORD ulErrorCode = 0;

    SetDefaultParameters();

    PrintSettings();

    if((lResult = OpenDevice(&ulErrorCode))!=MMC_SUCCESS)
    {
        LogError("OpenDevice", lResult, ulErrorCode);
        return lResult;
    }

    if((lResult = PrintDeviceVersion()) != MMC_SUCCESS)
    {
        LogError("PrintDeviceVersion", lResult, ulErrorCode);
        return lResult;
    }

    if((lResult = PrepareDemo(&ulErrorCode))!=MMC_SUCCESS)
    {
        LogError("PrepareDemo", lResult, ulErrorCode);
        return lResult;
    }

    if((lResult = Demo(&ulErrorCode))!=MMC_SUCCESS)
    {
        LogError("Demo", lResult, ulErrorCode);
        return lResult;
    }

    if((lResult = CloseDevice(&ulErrorCode))!=MMC_SUCCESS)
    {
        LogError("CloseDevice", lResult, ulErrorCode);
        return lResult;
    }
}

void SetDefaultParameters()
{
    //USB
    g_usNodeId = 1;
    g_deviceName = "EPOS4";
    g_protocolStackName = "MAXON SERIAL V2";
    g_interfaceName = "USB";
    g_portName = "USB0";
    g_baudrate = 1000000;
}

int OpenDevice(DWORD* p_pErrorCode)
{
    int lResult = MMC_FAILED;

    char* pDeviceName = new char[255];
    char* pProtocolStackName = new char[255];
    char* pInterfaceName = new char[255];
    char* pPortName = new char[255];

    strcpy(pDeviceName, g_deviceName.c_str());
    strcpy(pProtocolStackName, g_protocolStackName.c_str());
    strcpy(pInterfaceName, g_interfaceName.c_str());
    strcpy(pPortName, g_portName.c_str());

    cout << "Opening device..." << endl;

    g_pKeyHandle = VCS_OpenDevice(pDeviceName, pProtocolStackName, pInterfaceName, pPortName, p_pErrorCode);

    if(g_pKeyHandle!=0 && *p_pErrorCode == 0)
    {
        DWORD lBaudrate = 0;
        DWORD lTimeout = 0;

        if(VCS_GetProtocolStackSettings(g_pKeyHandle, &lBaudrate, &lTimeout, p_pErrorCode)!=0)
        {
            if(VCS_SetProtocolStackSettings(g_pKeyHandle, g_baudrate, lTimeout, p_pErrorCode)!=0)
            {
                if(VCS_GetProtocolStackSettings(g_pKeyHandle, &lBaudrate, &lTimeout, p_pErrorCode)!=0)
                {
                    if(g_baudrate==(int)lBaudrate)
                    {
                        lResult = MMC_SUCCESS;
                    }
                }
            }
        }
    }
    else
    {
        g_pKeyHandle = 0;
    }

    delete []pDeviceName;
    delete []pProtocolStackName;
    delete []pInterfaceName;
    delete []pPortName;

    return lResult;
}

int CloseDevice(DWORD* p_pErrorCode)
{
    int lResult = MMC_FAILED;

    *p_pErrorCode = 0;

    cout << "Closing device" << endl;

    if(VCS_CloseDevice(g_pKeyHandle, p_pErrorCode)!=0 && *p_pErrorCode == 0)
    {
        lResult = MMC_SUCCESS;
    }

    return lResult;
}

int PrepareDemo(DWORD * p_pErrorCode)
{
    int lResult = MMC_SUCCESS;
    BOOL oIsFault = 0;

    if(VCS_GetFaultState(g_pKeyHandle, g_usNodeId, &oIsFault, p_pErrorCode ) == 0)
    {
        LogError("VCS_GetFaultState", lResult, *p_pErrorCode);
        lResult = MMC_FAILED;
    }

    if(lResult==0)
    {
        if(oIsFault)
        {
            stringstream msg;
            msg << "clear fault, node = '" << g_usNodeId << "'";
            cout << msg.str() << endl;

            if(VCS_ClearFault(g_pKeyHandle, g_usNodeId, p_pErrorCode) == 0)
            {
                LogError("VCS_ClearFault", lResult, *p_pErrorCode);
                lResult = MMC_FAILED;
            }
        }

        if(lResult==0)
        {
            BOOL oIsEnabled = 0;

            if(VCS_GetEnableState(g_pKeyHandle, g_usNodeId, &oIsEnabled, p_pErrorCode) == 0)
            {
                LogError("VCS_GetEnableState", lResult, *p_pErrorCode);
                lResult = MMC_FAILED;
            }

            if(lResult==0)
            {
                if(!oIsEnabled)
                {
                    if(VCS_SetEnableState(g_pKeyHandle, g_usNodeId, p_pErrorCode) == 0)
                    {
                        LogError("VCS_SetEnableState", lResult, *p_pErrorCode);
                        lResult = MMC_FAILED;
                    }
                }
            }
        }
    }
    return lResult;
}

int DemoProfileVelocityMode(HANDLE p_DeviceHandle, WORD p_usNodeId, DWORD & p_rlErrorCode)
{
    int lResult = MMC_SUCCESS;
    stringstream msg;

    msg << "set profile velocity mode, node = " << p_usNodeId;

    cout << msg.str() << endl;

    if(VCS_ActivateProfileVelocityMode(p_DeviceHandle, p_usNodeId, &p_rlErrorCode) == 0)
    {
        LogError("VCS_ActivateProfileVelocityMode", lResult, p_rlErrorCode);
        lResult = MMC_FAILED;
    }
    else
    {
        list<long> velocityList;

        velocityList.push_back(5000);
        velocityList.push_back(10000);
        velocityList.push_back(20000);

        for(list<long>::iterator it = velocityList.begin(); it !=velocityList.end(); it++)
        {
            long targetvelocity = (*it);

            stringstream msg;
            msg << "move with target velocity = " << targetvelocity << " rpm, node = " << p_usNodeId;
            cout << msg.str() << endl;

            if(VCS_MoveWithVelocity(p_DeviceHandle, p_usNodeId, targetvelocity, &p_rlErrorCode) == 0)
            {
                lResult = MMC_FAILED;
                LogError("VCS_MoveWithVelocity", lResult, p_rlErrorCode);
                break;
            }

            Sleep(1000);
        }

        if(lResult == MMC_SUCCESS)
        {
            cout << "halt velocity movement" << endl;

            if(VCS_HaltVelocityMovement(p_DeviceHandle, p_usNodeId, &p_rlErrorCode) == 0)
            {
                lResult = MMC_FAILED;
                LogError("VCS_HaltVelocityMovement", lResult, p_rlErrorCode);
            }
        }
    }

    return lResult;
}

int DemoProfilePositionMode(HANDLE p_DeviceHandle, WORD p_usNodeId, DWORD & p_rlErrorCode)
{
    int lResult = MMC_SUCCESS;
    stringstream msg;

    msg << "set profile position mode, node = " << p_usNodeId;
    cout << msg.str() << endl;

    if(VCS_ActivateProfilePositionMode(p_DeviceHandle, p_usNodeId, &p_rlErrorCode) == 0)
    {
        LogError("VCS_ActivateProfilePositionMode", lResult, p_rlErrorCode);
        lResult = MMC_FAILED;
    }
    else
    {
        list<long> positionList;

        positionList.push_back(5000);
        positionList.push_back(-10000);
        positionList.push_back(5000);

        for(list<long>::iterator it = positionList.begin(); it !=positionList.end(); it++)
        {
            long targetPosition = (*it);
            stringstream msg;
            msg << "move to position = " << targetPosition << ", node = " << p_usNodeId;
            cout << msg.str() << endl;

            if(VCS_MoveToPosition(p_DeviceHandle, p_usNodeId, targetPosition, 0, 1, &p_rlErrorCode) == 0)
            {
                LogError("VCS_MoveToPosition", lResult, p_rlErrorCode);
                lResult = MMC_FAILED;
                break;
            }

            Sleep(1000);
        }

        if(lResult == MMC_SUCCESS)
        {
            cout << "halt position movement" << endl;

            if(VCS_HaltPositionMovement(p_DeviceHandle, p_usNodeId, &p_rlErrorCode) == 0)
            {
                LogError("VCS_HaltPositionMovement", lResult, p_rlErrorCode);
                lResult = MMC_FAILED;
            }
        }
    }

    return lResult;
}

int Demo(DWORD * p_pErrorCode)
{
    int lResult = MMC_SUCCESS;
    DWORD lErrorCode = 0;

    lResult = DemoProfileVelocityMode(g_pKeyHandle, g_usNodeId, lErrorCode);

    if(lResult != MMC_SUCCESS)
    {
        LogError("DemoProfileVelocityMode", lResult, lErrorCode);
    }
    else
    {
        lResult = DemoProfilePositionMode(g_pKeyHandle, g_usNodeId, lErrorCode);

        if(lResult != MMC_SUCCESS)
        {
            LogError("DemoProfilePositionMode", lResult, lErrorCode);
        }
        else
        {
            if(VCS_SetDisableState(g_pKeyHandle, g_usNodeId, &lErrorCode) == 0)
            {
                LogError("VCS_SetDisableState", lResult, lErrorCode);
                lResult = MMC_FAILED;
            }
        }
    }

    return lResult;
}


void SeparatorLine()
{
    const int lineLength = 65;
    for(int i=0; i<lineLength; i++)
    {
        cout << "-";
    }
    cout << endl;
}

int PrintAvailableInterfaces()
{
    int lResult = MMC_FAILED;
    int lStartOfSelection = 1;
    int lMaxStrSize = 255;
    char* pInterfaceNameSel = new char[lMaxStrSize];
    int lEndOfSelection = 0;
    DWORD ulErrorCode = 0;

    do
    {
        if(!VCS_GetInterfaceNameSelection((char*)g_deviceName.c_str(), (char*)g_protocolStackName.c_str(), lStartOfSelection, pInterfaceNameSel, lMaxStrSize, &lEndOfSelection, &ulErrorCode))
        {
            lResult = MMC_FAILED;
            LogError("GetInterfaceNameSelection", lResult, ulErrorCode);
            break;
        }
        else
        {
            lResult = MMC_SUCCESS;

            printf("interface = %s\n", pInterfaceNameSel);

            PrintAvailablePorts(pInterfaceNameSel);
        }

        lStartOfSelection = 0;
    }
    while(lEndOfSelection == 0);

    SeparatorLine();

    delete[] pInterfaceNameSel;

    return lResult;
}

int PrintAvailablePorts(char* p_pInterfaceNameSel)
{
    int lResult = MMC_FAILED;
    int lStartOfSelection = 1;
    int lMaxStrSize = 255;
    char* pPortNameSel = new char[lMaxStrSize];
    int lEndOfSelection = 0;
    DWORD ulErrorCode = 0;

    do
    {
        if(!VCS_GetPortNameSelection((char*)g_deviceName.c_str(), (char*)g_protocolStackName.c_str(), p_pInterfaceNameSel, lStartOfSelection, pPortNameSel, lMaxStrSize, &lEndOfSelection, &ulErrorCode))
        {
            lResult = MMC_FAILED;
            LogError("GetPortNameSelection", lResult, ulErrorCode);
            break;
        }
        else
        {
            lResult = MMC_SUCCESS;
            printf("            port = %s\n", pPortNameSel);
        }

        lStartOfSelection = 0;
    }
    while(lEndOfSelection == 0);

    return lResult;
}

int PrintDeviceVersion()
{
    int lResult = MMC_FAILED;
    WORD usHardwareVersion = 0;
    WORD usSoftwareVersion = 0;
    WORD usApplicationNumber = 0;
    WORD usApplicationVersion = 0;
    DWORD ulErrorCode = 0;

    if(VCS_GetVersion(g_pKeyHandle, g_usNodeId, &usHardwareVersion, &usSoftwareVersion, &usApplicationNumber, &usApplicationVersion, &ulErrorCode))
    {
        printf("%s Hardware Version    = 0x%04x\n      Software Version    = 0x%04x\n      Application Number  = 0x%04x\n      Application Version = 0x%04x\n",
               g_deviceName.c_str(), usHardwareVersion, usSoftwareVersion, usApplicationNumber, usApplicationVersion);
        lResult = MMC_SUCCESS;
    }

    return lResult;
}

void PrintSettings()
{
    stringstream msg;

    msg << "default settings:" << endl;
    msg << "node id             = " << g_usNodeId << endl;
    msg << "device name         = '" << g_deviceName << "'" << endl;
    msg << "protocal stack name = '" << g_protocolStackName << "'" << endl;
    msg << "interface name      = '" << g_interfaceName << "'" << endl;
    msg << "port name           = '" << g_portName << "'"<< endl;
    msg << "baudrate            = " << g_baudrate;

    cout << msg.str() << endl;

    SeparatorLine();
}

void LogError(string functionName, int p_lResult, DWORD p_ulErrorCode)
{
    cerr << g_programName << ": " << functionName << " failed (result=" << p_lResult << ", errorCode=0x" << std::hex << p_ulErrorCode << ")"<< endl;
}

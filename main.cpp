#include <Definitions.h>
#include <string>
#include <iostream>
#include <sstream>
#include <list>
#include "tinyxml2.h"

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

// Testing parameters
DWORD g_V_MODE_velocity = 10000;       // rpm
DWORD g_V_MODE_acceleration = 10000;   // unit: rpm/s
DWORD g_V_MODE_deceleration = 10000;
int g_V_MODE_duration = 0;

DWORD g_P_MODE_position = 0;
DWORD g_P_MODE_max_velocity = 0;
DWORD g_P_MODE_acceleration = 0;
DWORD g_P_MODE_deceleration = 0;


void SetDefaultParameters();
int OpenDevice(DWORD* p_pErrorCode);
int CloseDevice(DWORD* p_pErrorCode);
int PrepareDemo(DWORD * p_pErrorCode);
int SetupProfileVelocityMode(HANDLE p_DeviceHandle, WORD p_usNodeId, DWORD & p_rlErrorCode);
int SetupProfilePositionMode(HANDLE p_DeviceHandle, WORD p_usNodeId, DWORD & p_rlErrorCode);
int DemoProfileVelocityMode(HANDLE p_DeviceHandle, WORD p_usNodeId, DWORD & p_rlErrorCode);
int DemoProfilePositionMode(HANDLE p_DeviceHandle, WORD p_usNodeId, DWORD & p_rlErrorCode);
int zeroPosition(HANDLE p_DeviceHandle, WORD p_usNodeId, DWORD & p_rlErrorCode);
void PrintMotorPositon(HANDLE p_DeviceHandle, WORD p_usNodeId, DWORD & p_rlErrorCode);
int PrintAvailableInterfaces();
int PrintAvailablePorts(char* p_pInterfaceNameSel);
int PrintDeviceVersion();
void  PrintSettings();
void LogError(string functionName, int p_lResult, DWORD p_ulErrorCode);

enum Mode { POS_MODE, VEL_MODE};

int main(int argc, char** argv) {
    // Load parameters and setting from XML
    tinyxml2::XMLDocument xml_doc;
    xml_doc.LoadFile("config.xml");
    tinyxml2::XMLNode * pRoot = xml_doc.FirstChild();
    tinyxml2::XMLElement * pElement = pRoot->FirstChildElement("MODE");

    Mode mode;

    if (pElement == nullptr) return tinyxml2::XML_ERROR_PARSING_ELEMENT;
    int iOutInt;
    string str_mode = string(pElement->GetText());
    cout << "Current Active Mode: " << str_mode  << " Mode" << endl;

    pElement = pRoot->FirstChildElement("Iteration");
    int iteration = pElement->IntText(1);
    cout << "Number of iteration: " << iteration << endl;

    if (str_mode.compare("Velocity") == 0) {
        mode = VEL_MODE;
        cout << "--- Loading parameters for Velocity Mode ---" << endl;

        tinyxml2::XMLElement * pParams = pRoot->FirstChildElement("Velocity_Mode_Parameters");
        tinyxml2::XMLElement * pTarVelocity = pParams->FirstChildElement("Target_Velocity");
        tinyxml2::XMLElement * pAcc = pParams->FirstChildElement("Acceleration");
        tinyxml2::XMLElement * pDec = pParams->FirstChildElement("Deceleration");
        tinyxml2::XMLElement * pDur = pParams->FirstChildElement("Duration");
        double v = 0.0, acc = 0.0, dec = 0.0;
        pTarVelocity->QueryDoubleText(&v);
        pAcc->QueryDoubleText(&acc);
        pDec->QueryDoubleText(&dec);
        pDur->QueryIntText(&g_V_MODE_duration);
        cout << "Target Velocity: " << v << " rpm" << endl;
        cout << "Acceleration: " << acc << " rpm/s" << endl;
        cout << "Deceleration: " << dec << " rpm/s" << endl;
        g_V_MODE_velocity = static_cast<DWORD>(v);
        g_V_MODE_acceleration = static_cast<DWORD>(acc);
        g_V_MODE_deceleration = static_cast<DWORD>(dec);
        g_V_MODE_duration *= 1000; // second to ms
        cout << "g_V_MODE_duration: " << g_V_MODE_duration << endl;
    }
    else if (str_mode.compare("Position") == 0) {
        mode = POS_MODE;
        cout << "Loading parameters for Position Mode" << endl;

        tinyxml2::XMLElement * pParams = pRoot->FirstChildElement("Position_Mode_Parameters");
        tinyxml2::XMLElement * pTarPosition = pParams->FirstChildElement("Target_Position");
        tinyxml2::XMLElement * pMaxVelocity = pParams->FirstChildElement("Max_Velocity");
        tinyxml2::XMLElement * pAcc = pParams->FirstChildElement("Acceleration");
        tinyxml2::XMLElement * pDec = pParams->FirstChildElement("Deceleration");
        double p = 0.0, max_v = 0.0, acc = 0.0, dec = 0.0;
        pTarPosition->QueryDoubleText(&p);
        pMaxVelocity->QueryDoubleText(&max_v);
        pAcc->QueryDoubleText(&acc);
        pDec->QueryDoubleText(&dec);
        cout << "Target Position: " << p << endl;
        cout << "Max Velocity: " << max_v << " rpm" << endl;
        cout << "Acceleration: " << acc << " rpm/s" << endl;
        cout << "Deceleration: " << dec << " rpm/s" << endl;
        g_P_MODE_position = static_cast<DWORD>(p);
        g_P_MODE_max_velocity = static_cast<DWORD>(max_v);
        g_P_MODE_acceleration = static_cast<DWORD>(acc);
        g_P_MODE_deceleration = static_cast<DWORD>(dec);
    }
    else {
        cout << "Error: Unknown mode" << endl;
        return -1;
    }


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

//    if((lResult = Demo(&ulErrorCode))!=MMC_SUCCESS)
//    {
//        LogError("Demo", lResult, ulErrorCode);
//        return lResult;
//    }
    DWORD lErrorCode = 0;

    zeroPosition(g_pKeyHandle, g_usNodeId, lErrorCode);
    cout << "<Begin> ";
    PrintMotorPositon(g_pKeyHandle, g_usNodeId, lErrorCode);

    if (mode == VEL_MODE) {
        int acc_interval = static_cast<int>(((double)g_V_MODE_velocity / (double)g_V_MODE_acceleration) * 1000);
        int dec_interval = static_cast<int>(((double)g_V_MODE_velocity / (double)g_V_MODE_deceleration) * 1000);
        lResult = SetupProfileVelocityMode(g_pKeyHandle, g_usNodeId, lErrorCode);
        if(lResult != MMC_SUCCESS) {
            LogError("SetupProfileVelocityMode", lResult, lErrorCode);
        }
        else {
            for (int i = 0; i < iteration; ++i) {
                cout << "Iteration: #" << i+1 << "/" << iteration << endl;
                long targetvelocity = g_V_MODE_velocity;

                stringstream msg;
                msg << "move with target velocity = " << targetvelocity << " rpm, node = " << g_usNodeId;
                cout << msg.str() << endl;

                if(VCS_MoveWithVelocity(g_pKeyHandle, g_usNodeId, targetvelocity, &lErrorCode) == 0)
                {
                    lResult = MMC_FAILED;
                    LogError("VCS_MoveWithVelocity", lResult, lErrorCode);
                }
                Sleep(static_cast<DWORD>(g_V_MODE_duration + acc_interval));

                if(VCS_MoveWithVelocity(g_pKeyHandle, g_usNodeId, 0, &lErrorCode) == 0)
                {
                    lResult = MMC_FAILED;
                    LogError("VCS_MoveWithVelocity", lResult, lErrorCode);
                }
                Sleep(static_cast<DWORD>(dec_interval));

                targetvelocity = -g_V_MODE_velocity;
                msg.str("");
                msg.clear();
                msg << "move with target velocity = " << targetvelocity << " rpm, node = " << g_usNodeId;
                cout << msg.str() << endl;

                if(VCS_MoveWithVelocity(g_pKeyHandle, g_usNodeId, targetvelocity, &lErrorCode) == 0)
                {
                    lResult = MMC_FAILED;
                    LogError("VCS_MoveWithVelocity", lResult, lErrorCode);
                }
                Sleep(static_cast<DWORD>(g_V_MODE_duration + acc_interval));

                if(VCS_MoveWithVelocity(g_pKeyHandle, g_usNodeId, 0, &lErrorCode) == 0)
                {
                    lResult = MMC_FAILED;
                    LogError("VCS_MoveWithVelocity", lResult, lErrorCode);
                }
                Sleep(static_cast<DWORD>(dec_interval));
            }
        }
        if(lResult == MMC_SUCCESS)
        {
            cout << "halt velocity movement" << endl;

            if(VCS_HaltVelocityMovement(g_pKeyHandle, g_usNodeId, &lErrorCode) == 0)
            {
                lResult = MMC_FAILED;
                LogError("VCS_HaltVelocityMovement", lResult, lErrorCode);
            }
        }

//        cout << "<Before Home> ";
//        PrintMotorPositon(g_pKeyHandle, g_usNodeId, lErrorCode);
//        //Start homing
//        if(lResult == MMC_SUCCESS)
//        {
//            VCS_ActivatePositionMode(g_pKeyHandle, g_usNodeId, &lErrorCode);
//            VCS_SetHomingParameter(g_pKeyHandle, g_usNodeId, 1000, 100, 10, (long)0, 100, 0, &lErrorCode);
//            if(VCS_FindHome(g_pKeyHandle, g_usNodeId, HM_ACTUAL_POSITION, &lErrorCode) == 0)
//            {
//                lResult = MMC_FAILED;
//                LogError("VCS_WaitForHomingAttained", lResult, lErrorCode);
//            }
//        }
//        if(VCS_WaitForHomingAttained(g_pKeyHandle, g_usNodeId, 5000, &lErrorCode) == 0)
//        {
//            lResult = MMC_FAILED;
//            LogError("VCS_WaitForHomingAttained", lResult, lErrorCode);
//        }
//        cout << "<After Home> ";
//        PrintMotorPositon(g_pKeyHandle, g_usNodeId, lErrorCode);
    }
    else if (mode == POS_MODE) {
        double acc_interval = ((double)g_P_MODE_max_velocity / (double)g_P_MODE_acceleration) * 1000.0;
        double dec_interval = ((double)g_P_MODE_max_velocity / (double)g_P_MODE_deceleration) * 1000.0;
        int oneside_time = 1.1* static_cast<int>(((double)g_P_MODE_position / (double)g_P_MODE_max_velocity) + 0.5 * (acc_interval + dec_interval));
        lResult = SetupProfilePositionMode(g_pKeyHandle, g_usNodeId, lErrorCode);
        if(lResult != MMC_SUCCESS) {
            LogError("SetupProfileVelocityMode", lResult, lErrorCode);
        }
        else {
            for (int i = 0; i < iteration; ++i) {
                cout << "Iteration: #" << i + 1 << "/" << iteration << endl;
                long targetPosition = static_cast<long>(g_P_MODE_position);
                stringstream msg;
                msg << "move to position = " << targetPosition << ", node = " << g_usNodeId;
                cout << msg.str() << endl;

                if (VCS_MoveToPosition(g_pKeyHandle, g_usNodeId, targetPosition, true, 1, &lErrorCode) == 0) {
                    LogError("VCS_MoveToPosition", lResult, lErrorCode);
                    lResult = MMC_FAILED;
                }
                if (i == 0)
                    Sleep(static_cast<DWORD>(oneside_time));
                else
                    Sleep(static_cast<DWORD>(2*oneside_time));

                targetPosition = -static_cast<long>(g_P_MODE_position);
                msg.str("");
                msg.clear();
                msg << "move to position = " << targetPosition << ", node = " << g_usNodeId;
                cout << msg.str() << endl;

                if (VCS_MoveToPosition(g_pKeyHandle, g_usNodeId, targetPosition, true, 1, &lErrorCode) == 0) {
                    LogError("VCS_MoveToPosition", lResult, lErrorCode);
                    lResult = MMC_FAILED;
                }
                Sleep(static_cast<DWORD>(2*oneside_time));
            }
            if (VCS_MoveToPosition(g_pKeyHandle, g_usNodeId, 0, true, 1, &lErrorCode) == 0) {
                LogError("VCS_MoveToPosition", lResult, lErrorCode);
                lResult = MMC_FAILED;
            }
            Sleep(static_cast<DWORD>(oneside_time));

            if(lResult == MMC_SUCCESS)
            {
                cout << "halt position movement" << endl;

                if(VCS_HaltPositionMovement(g_pKeyHandle, g_usNodeId, &lErrorCode) == 0)
                {
                    LogError("VCS_HaltPositionMovement", lResult, lErrorCode);
                    lResult = MMC_FAILED;
                }
            }
        }
    }
//    lResult = DemoProfileVelocityMode(g_pKeyHandle, g_usNodeId, lErrorCode);
//
//    if(lResult != MMC_SUCCESS)
//    {
//        LogError("DemoProfileVelocityMode", lResult, lErrorCode);
//    }
//    else
//    {
//        lResult = DemoProfilePositionMode(g_pKeyHandle, g_usNodeId, lErrorCode);
//
//        if(lResult != MMC_SUCCESS)
//        {
//            LogError("DemoProfilePositionMode", lResult, lErrorCode);
//        }
//        else
//        {
//            if(VCS_SetDisableState(g_pKeyHandle, g_usNodeId, &lErrorCode) == 0)
//            {
//                LogError("VCS_SetDisableState", lResult, lErrorCode);
//                lResult = MMC_FAILED;
//            }
//        }
//    }

    if(VCS_SetDisableState(g_pKeyHandle, g_usNodeId, &lErrorCode) == 0)
    {
        LogError("VCS_SetDisableState", lResult, lErrorCode);
        lResult = MMC_FAILED;
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

int SetupProfileVelocityMode(HANDLE p_DeviceHandle, WORD p_usNodeId, DWORD & p_rlErrorCode)
{
    int lResult = MMC_SUCCESS;
    stringstream msg;

    msg << "set profile velocity mode, node = " << p_usNodeId;

    cout << msg.str() << endl;

    if(VCS_ActivateProfileVelocityMode(p_DeviceHandle, p_usNodeId, &p_rlErrorCode) == 0) {
        lResult = MMC_FAILED;
        LogError("VCS_ActivateProfileVelocityMode", lResult, p_rlErrorCode);
    } else {
        if(VCS_SetVelocityProfile(p_DeviceHandle, p_usNodeId, g_V_MODE_acceleration, g_V_MODE_deceleration, &p_rlErrorCode) == 0)
        {
            lResult = MMC_FAILED;
            LogError("VCS_SetVelocityProfile", lResult, p_rlErrorCode);
        }
    }

    return lResult;
}

int SetupProfilePositionMode(HANDLE p_DeviceHandle, WORD p_usNodeId, DWORD & p_rlErrorCode)
{
    int lResult = MMC_SUCCESS;

    stringstream msg;
    msg << "set profile position mode, node = " << p_usNodeId;
    cout << msg.str() << endl;

    if(VCS_ActivateProfilePositionMode(p_DeviceHandle, p_usNodeId, &p_rlErrorCode) == 0) {
        lResult = MMC_FAILED;
        LogError("VCS_ActivateProfileVelocityMode", lResult, p_rlErrorCode);
    } else {
        if(VCS_SetPositionProfile(p_DeviceHandle, p_usNodeId, g_P_MODE_max_velocity, g_P_MODE_acceleration, g_P_MODE_deceleration, &p_rlErrorCode) == 0)
        {
            lResult = MMC_FAILED;
            LogError("VCS_SetPositionProfile", lResult, p_rlErrorCode);
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
        lResult = MMC_FAILED;
        LogError("VCS_ActivateProfilePositionMode", lResult, p_rlErrorCode);
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
                lResult = MMC_FAILED;
                LogError("VCS_MoveToPosition", lResult, p_rlErrorCode);
                break;
            }

            Sleep(1000);
        }

        if(lResult == MMC_SUCCESS)
        {
            cout << "halt position movement" << endl;

            if(VCS_HaltPositionMovement(p_DeviceHandle, p_usNodeId, &p_rlErrorCode) == 0)
            {
                lResult = MMC_FAILED;
                LogError("VCS_HaltPositionMovement", lResult, p_rlErrorCode);
            }
        }
    }

    return lResult;
}

void PrintMotorPositon(HANDLE p_DeviceHandle, WORD p_usNodeId, DWORD & p_rlErrorCode) {
    long pos;
    VCS_GetPositionIs(p_DeviceHandle, p_usNodeId, &pos, &p_rlErrorCode);
    cout << "Motor node #" <<  p_usNodeId << " position: " << pos << endl;
}

// Set current position as zero (homing) position
int zeroPosition(HANDLE p_DeviceHandle, WORD p_usNodeId, DWORD & p_rlErrorCode) {
    int lResult = MMC_SUCCESS;
    DWORD lErrorCode = 0;

    if(VCS_DefinePosition(p_DeviceHandle, p_usNodeId, 0, &p_rlErrorCode) == 0)
    {
        LogError("VCS_DefinePosition", lResult, lErrorCode);
        lResult = MMC_FAILED;
    }
    return  lResult;
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

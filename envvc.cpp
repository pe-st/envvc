/**
 * @file
 * @brief Set the environment for Visual Studio
 *
 *         $Id: //depot.hugwi.ch/master/Tools/misc/envvc.cpp#5 $
 *     $Change: 26301 $
 *   $DateTime: 2007/01/17 11:00:46 $
 *     $Author: peter.steiner $
 * $Maintainer: peter.steiner $
 *    $Created: peter.steiner 2005/04/07 $
 *
 * Copyright Peter Steiner and Hug-Witschi AG 2005 - 2007.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

/* headers for this module ---------------------------------------------------*/

/* standard headers (ANSI, POSIX and HW standards) ---------------------------*/
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>    // for std::runtime_error
#include <stdlib.h>     // getenv, _putenv
#include <process.h>    // _spawnvp

/* headers from other modules ------------------------------------------------*/
#include <windows.h>


/*-----------------------------------------------------------------------------+
|   namespaces                                                                 |
+-----------------------------------------------------------------------------*/

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::runtime_error;

/*-----------------------------------------------------------------------------+
|   local constants, macros and enums                                          |
+-----------------------------------------------------------------------------*/

const string msDir("HKLM\\SOFTWARE\\Microsoft\\");
const string devDiv("HKLM\\SOFTWARE\\Microsoft\\DevDiv\\");
const string studioDir("HKLM\\SOFTWARE\\Microsoft\\VisualStudio\\");
const string expressDir("HKLM\\SOFTWARE\\Microsoft\\VCExpress\\");

const string banner("envvc - environment tool for Visual C++ X.Y\n"
                    "    (c) 2005-2007 Peter Steiner and Hug-Witschi AG\n");

/*-----------------------------------------------------------------------------+
|   local types                                                                |
+-----------------------------------------------------------------------------*/


class RegistryKey {
public:
    explicit RegistryKey(const std::string& key);
    ~RegistryKey();

    std::string asString(const std::string& name) const;
    DWORD asDword(const std::string& name) const;

    static std::string getString(const std::string& key,
                                 const std::string& valueName);
    static DWORD getDword(const std::string& key,
                          const std::string& valueName);
private:
    HKEY keyHandle_;
};


/*-----------------------------------------------------------------------------+
|   declaration of local (static) functions                                    |
+-----------------------------------------------------------------------------*/

static void printUsage();
static bool doVC6();
static bool doVC71();
static bool doVC80(bool useFX);

static std::string trimmedString(const std::string& key,
                                 const std::string& valueName);

static std::string getEnv(const std::string& var);
static void putEnv(const std::string& var, const std::string& value);

/*-----------------------------------------------------------------------------+
|   module global variables                                                    |
+-----------------------------------------------------------------------------*/

string envCollection;
string compiler;

/*-----------------------------------------------------------------------------+
|   functions                                                                  |
+-----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*/
int main (int argc, char* argv[])
{
    int retval = 1;
    try {
        bool isVerbose = false;
        bool isForced = false;
        bool useFX = false;
        bool foundValidOption = true;
        while (argc > 1 && foundValidOption)
        {
            string arg1(argv[1]);
            if (arg1 == "-v")
            {
                isVerbose = true;
                --argc;
                ++argv;
            }
            else if (arg1 == "-f")
            {
                isForced = true;
                --argc;
                ++argv;
            }
            else if (arg1 == "fx")
            {
                useFX = true;
                --argc;
                ++argv;
            }
            else
                foundValidOption = false;
        }

        if (argc <= 1)
        {
            printUsage();
            exit(1);
        }

        string version(argv[1]);
        bool isCurrent;
        if (version == "6" || version == "60")
            isCurrent = doVC6();
        else if (version == "71")
            isCurrent = doVC71();
        else if (version == "80")
            isCurrent = doVC80(useFX);
        else
        {
            printUsage();
            exit(1);
        }

        if (useFX && version != "80")
        {
            cout << "Option 'fx' not supported for this version ("
                 << compiler << ")." << endl;
        }

        if (!isForced && !isCurrent)
        {
            cout << "Please install the lastest Service Pack or use option '-f'" << endl;
            exit(1);
        }

        if (isVerbose)
        {
            cout << banner
                 << "Detected: " << compiler << endl;
        }

        if (argc > 2)
        {
            retval = static_cast<int>(_spawnvp(_P_WAIT, argv[2], argv+2));
            if (retval == -1)
            {
                cout << "failed to execute " << argv[2] << ": errno " << errno << ", \""
                     << strerror(errno) << "\"\n";
            }
        }
        else
        {
            cout << envCollection << endl;
            retval = 0;
        }
    }
    catch (const std::exception& e)
    {
        cerr << "exception: " << e.what() << "\n";
        return 1;
    }
    catch (...)
    {
        cerr << "some exception happened\n";
        return 1;
    }

    return retval;
}


/*-----------------------------------------------------------------------------+
|   local (static) functions                                                   |
+-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static void printUsage()
{
    cout << banner
         << "    usage: envvc [-v] [-f] [fx] 6|60|71|80 [command...]\n"
         << "    -v      : verbose. Print the detected compiler version.\n"
         << "    -f      : force execution even w/o the latest service pack\n"
         << "    fx      : use the .NET 3 SDK (formerly WinFX)\n"
         << "    command : command to execute within the changed environment\n"
         << endl;
}

/*----------------------------------------------------------------------------*/
static bool doVC6()
{
    string vc98    = trimmedString(studioDir + "6.0\\Setup\\Microsoft Visual C++",
                                   "ProductDir");
    string vsDir   = trimmedString(studioDir + "6.0\\Setup\\Microsoft Visual Studio",
                                   "ProductDir");
    string common6 = trimmedString(studioDir + "6.0\\Setup",
                                   "VsCommonDir");

    // these are taken from
    // "C:\Programme\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"
    // (but in the batch they are in the 8.3 shortened form...)
    putEnv("MSDevDir", common6 + "\\msdev98");
    putEnv("MSVCDir", vc98);

    string oldpath = getEnv("PATH");
    string oldinc = getEnv("INCLUDE");
    string oldlib = getEnv("LIB");

    string newpath
        = common6 + "\\msdev98\\bin;"
        + vc98 + "\\bin;"
        + common6 + "\\tools\\winnt;"
        + common6 + "\\tools;"
        + oldpath;
    string newinc
        = vc98 + "\\atl\\include;"
        + vc98 + "\\include;"
        + vc98 + "\\mfc\\include;"
        + oldinc;
    string newlib
        = vc98 + "\\lib;"
        + vc98 + "\\mfc\\lib;"
        + oldlib;
    putEnv("PATH", newpath);
    putEnv("INCLUDE", newinc);
    putEnv("LIB", newlib);

    // these are new, but needed for v86.mak
    putEnv("VCINSTALLDIR", vsDir);
    putEnv("VC_VERS", "60");

    DWORD sp = 0;
    try {
        sp = RegistryKey::getDword(studioDir + "6.0\\ServicePacks",
                                   "latest");
        compiler = "Visual C++ 6.0 SP " + string(1, static_cast<char>('0' + sp));
    }
    catch (runtime_error&)
    {
        compiler = "Visual C++ 6.0 (no ServicePack installed)";
    }

    // the current (2005-05-02) service pack is 6. Nobody should use older
    // versions!
    if (sp < 6)
    {
        // make the message look like an error message...
        cout << vsDir << "\\install.htm(1) : error SP: "
             << "there's a newer service pack available!" << endl;
        return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/
static bool doVC71()
{
    string instDir = trimmedString(studioDir + "7.1",
                                   "InstallDir");
    string vc7     = trimmedString(studioDir + "7.1\\Setup\\VC",
                                   "ProductDir");
    string vsDir   = trimmedString(studioDir + "7.1\\Setup\\VS",
                                   "ProductDir");
    string common7 = trimmedString(studioDir + "7.1\\Setup\\VS",
                                   "VS7CommonDir");
    string ideDir  = trimmedString(studioDir + "7.1\\Setup\\VS",
                                   "EnvironmentDirectory");
    string clrVers = trimmedString(studioDir + "7.1",
                                   "CLR Version");
    string clrRoot = trimmedString(msDir + ".NETFramework",
                                   "InstallRoot");
    string clrSdk  = trimmedString(msDir + ".NETFramework",
                                   "sdkInstallRootv1.1");

    // these are taken from
    // "C:\Programme\Microsoft Visual Studio .NET 2003\Common7\Tools\vsvars32.bat"
    putEnv("VSINSTALLDIR", instDir);
    putEnv("VCINSTALLDIR", vsDir);
    putEnv("FrameworkDir", clrRoot);
    putEnv("FrameworkVersion", clrVers);
    putEnv("FrameworkSDKDir", clrSdk);
    putEnv("DevEnvDir", ideDir);
    putEnv("MSVCDir", vc7);

    string oldpath = getEnv("PATH");
    string oldinc = getEnv("INCLUDE");
    string oldlib = getEnv("LIB");

    string newpath
        = ideDir + ";"
        + vc7 + "\\bin;"
        + common7 + "\\tools;"
        + common7 + "\\tools\\bin\\prerelease;"
        + common7 + "\\tools\\bin;"
        + clrSdk + "\\bin;"
        + clrRoot + "\\" + clrVers + ";"
        + oldpath;
    string newinc
        = vc7 + "\\atlmfc\\include;"
        + vc7 + "\\include;"
        + vc7 + "\\platformSDK\\include\\prerelease;"
        + vc7 + "\\platformSDK\\include;"
        + clrSdk + "\\include;"
        + oldinc;
    string newlib
        = vc7 + "\\atlmfc\\lib;"
        + vc7 + "\\lib;"
        + vc7 + "\\platformSDK\\lib\\prerelease;"
        + vc7 + "\\platformSDK\\lib;"
        + clrSdk + "\\lib;"
        + oldlib;
    putEnv("PATH", newpath);
    putEnv("INCLUDE", newinc);
    putEnv("LIB", newlib);

    // these are new, but needed for v86.mak
    putEnv("VC_VERS", "71");

    compiler = "Visual C++ 7.1";

    DWORD sp = 0;
    try {
        sp = RegistryKey::getDword(studioDir + "7.1\\Setup\\Servicing",
                                   "CurrentSPLevel");
        if (sp > 0)
            compiler += " SP " + string(1, static_cast<char>('0' + sp));
        else
            compiler += " (no ServicePack installed)";
    }
    catch (runtime_error&)
    {
        compiler += " (no ServicePack installed)";
    }

    // the current (2007-01-16) service pack is 1. Nobody should use older
    // versions!
    if (sp < 1)
    {
        // make the message look like an error message...
        cout << vsDir << "\\install.htm(1) : error SP: "
             << "there's a newer service pack available!" << endl;
        return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/
static bool doVC80(bool useFX)
{
    bool isExpress = false;
    string regDir = studioDir;
    string vc8;
    try
    {
        vc8 = trimmedString(regDir + "8.0\\Setup\\VC",
                            "ProductDir");
    }
    catch (runtime_error&)
    {
        regDir = expressDir;
        vc8 = trimmedString(regDir + "8.0\\Setup\\VC",
                            "ProductDir");
        isExpress = true;
    }

    string vsDir   = trimmedString(regDir + "8.0\\Setup\\VS",
                                   "ProductDir");

    string common7 = vsDir + "\\Common7";
    string ideDir = common7 + "\\IDE";
    if (!isExpress)
    {
        common7 = trimmedString(regDir + "8.0\\Setup\\VS",
                                "VS7CommonDir");
        ideDir  = trimmedString(regDir + "8.0\\Setup\\VS",
                                "EnvironmentDirectory");
    }

    string clrVers = trimmedString(regDir + "8.0",
                                   "CLR Version");
    string clrRoot = trimmedString(msDir + ".NETFramework",
                                   "InstallRoot");
    string clrSdk  = trimmedString(msDir + ".NETFramework",
                                   "sdkInstallRootv2.0");

    string msSdk = useFX
        ? trimmedString(msDir + "Microsoft SDKs\\Windows", "CurrentInstallFolder")
        : "";

    // these are taken from
    // "C:\Programme\Microsoft Visual Studio 8\Common7\Tools\vsvars32.bat"
    putEnv("VSINSTALLDIR", vsDir);
    putEnv("VCINSTALLDIR", vc8);
    putEnv("FrameworkDir", clrRoot);
    putEnv("FrameworkVersion", clrVers);
    putEnv("FrameworkSDKDir", clrSdk);
    putEnv("DevEnvDir", ideDir);

    string fxInc;
    if (useFX)
    {
        // these are taken from
        // "C:\Program Files\Microsoft SDKs\Windows\v6.0\Bin\SetEnv.Cmd"
        putEnv("MSSdk", msSdk);
        putEnv("SdkTools", msSdk + "\\Bin");
        putEnv("OSLibraries", msSdk + "\\Lib");
        fxInc = msSdk + "\\Include;" + msSdk + "\\Include\\gl";
        putEnv("OSIncludes", fxInc);
        putEnv("VCTools", msSdk + "\\VC\\Bin");
        putEnv("VCLibraries", msSdk + "\\VC\\Lib");
        putEnv("VCIncludes", msSdk + "\\VC\\Include;" + msSdk + "\\VC\\Include\\Sys");
        putEnv("ReferenceAssemblies", "%ProgramFiles%\\Reference Assemblies\\Microsoft\\WinFX\\v3.0");
    }

    string oldpath = getEnv("PATH");
    string oldinc = getEnv("INCLUDE");
    string oldlib = getEnv("LIB");

    string newpath
        = ideDir + ";"
        + (useFX ? (msSdk + "\\bin;") : "")
        + vc8 + "\\bin;"
        + (useFX ? "" : (vc8 + "\\platformSDK\\bin;"))
        + vc8 + "\\vcpackages;"
        + common7 + "\\tools;"
        + common7 + "\\tools\\bin;"
        + clrSdk + "\\bin;"
        + clrRoot + "\\" + clrVers + ";"
        + oldpath;
    string newinc
        = (useFX ? (fxInc + ";") : "")
        + vc8 + "\\atlmfc\\include;"
        + vc8 + "\\include;"
        + (useFX ? "" : (vc8 + "\\platformSDK\\include;"))
        + clrSdk + "\\include;"
        + oldinc;
    string newlib
        = (useFX ? (msSdk + "\\lib;") : "")
        + vc8 + "\\atlmfc\\lib;"
        + vc8 + "\\lib;"
        + (useFX ? "" : (vc8 + "\\platformSDK\\lib;"))
        + clrSdk + "\\lib;"
        + oldlib;
    putEnv("PATH", newpath);
    putEnv("INCLUDE", newinc);
    putEnv("LIB", newlib);

    putEnv("LIBPATH", clrRoot + "\\" + clrVers);

    // these are new, but needed for v86.mak
    putEnv("VC_VERS", "80");

    compiler = isExpress
        ? "Visual C++ 2005 Express"
        : "Visual C++ 8.0";

    DWORD sp = 0;
    try {
        sp = RegistryKey::getDword(devDiv + "VS\\Servicing\\8.0",
                                   "SP");
        if (sp > 0)
            compiler += " SP " + string(1, static_cast<char>('0' + sp));
        else
            compiler += " (no ServicePack installed)";
    }
    catch (runtime_error&)
    {
        compiler += " (no ServicePack installed)";
    }

    // the current (2007-01-16) service pack is 1. Nobody should use older
    // versions!
    if (sp < 1)
    {
        // make the message look like an error message...
        cout << vc8 << "\\install.htm(1) : error SP: "
             << "there's a newer service pack available!" << endl;
        return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/
/**
 * Reads a registry value and chops off trailing blanks and backslashes
 *
 * @param key       registry key
 * @param valueName name of the value
 *
 * @return the trimmed value
 */
static std::string trimmedString(const std::string& key,
                                 const std::string& valueName)
{
    std::string result = RegistryKey::getString(key, valueName);

    // chop off trailing blanks and backslashes
    string::size_type size = result.find_last_not_of("\\ ");
    if (size != string::npos)
        result.resize(size+1);

    return result;
}

/*----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------+
|   environment functions                                                      |
+-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static std::string getEnv(const std::string& var)
{
    char* value = getenv(var.c_str());
    if (value)
        return string(value);
    else
        return string();
}

/*----------------------------------------------------------------------------*/
static void putEnv(const std::string& var, const std::string& value)
{
    string putStr = var + "=" + value;
    if (_putenv(putStr.c_str()) != 0)
        throw runtime_error("_putenv failed");

    envCollection += putStr + "\n";
}

/*----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------+
|   RegistryKey methods                                                        |
+-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
RegistryKey::RegistryKey(const std::string& key)
    : keyHandle_(0)
{
    HKEY hkey = 0;

    // split the key in the top level key part and the rest
    string::size_type pos = key.find_first_of('\\');
    string toplevel = key.substr(0, pos);
    string regpath = key.substr(pos+1);

    if (toplevel == "HKLM")
        hkey = HKEY_LOCAL_MACHINE;
    else if (toplevel == "HKCU")
        hkey = HKEY_CURRENT_USER;
    else if (toplevel == "HKCR")
        hkey = HKEY_CLASSES_ROOT;
    else if (toplevel == "HKU")
        hkey = HKEY_USERS;

    LONG result = RegOpenKeyEx(hkey,
                               regpath.c_str(),
                               NULL,
                               KEY_READ, // read access is enough for now
                               &keyHandle_);

    if (result != ERROR_SUCCESS || keyHandle_ == 0)
        throw runtime_error("Could not open " + key);
}

/*----------------------------------------------------------------------------*/
RegistryKey::~RegistryKey()
{
    if (keyHandle_)
    {
        RegCloseKey(keyHandle_);
        keyHandle_ = 0;
    }
}

/*----------------------------------------------------------------------------*/
std::string RegistryKey::asString(const std::string& name) const
{
    DWORD type;
    DWORD size;

    // two steps: first find out the size of the data
    LONG result = RegQueryValueEx(keyHandle_,
                                  name.c_str(), NULL,
                                  &type,
                                  NULL, &size);

    if (result != ERROR_SUCCESS)
        throw runtime_error("Could not get size of " + name);
    if (type != REG_SZ && type != REG_EXPAND_SZ)
        throw runtime_error("Not a string: " + name);

    // now get the real data
    vector<char> buffer(size);
    result = RegQueryValueEx(keyHandle_,
                             name.c_str(), NULL,
                             &type,
                             reinterpret_cast<LPBYTE>(&buffer[0]),
                             &size);

    if (result != ERROR_SUCCESS)
        throw runtime_error("Could not query " + name);

    if (type == REG_SZ)
    {
        return string(&buffer[0]);
    }
    else if (type == REG_SZ)
    {
        // use ExpandEnvironmentStrings here?
        return string(&buffer[0]);
    }
    else
        throw runtime_error("Unexpected type: " + name);
}

/*----------------------------------------------------------------------------*/
DWORD RegistryKey::asDword(const std::string& name) const
{
    DWORD type;
    DWORD value;
    DWORD size = 4;

    LONG result = RegQueryValueEx(keyHandle_,
                                  name.c_str(), NULL,
                                  &type,
                                  reinterpret_cast<LPBYTE>(&value),
                                  &size);

    if (result != ERROR_SUCCESS)
        throw runtime_error("Could not read " + name);
    if (type != REG_DWORD)
        throw runtime_error("Not a DWORD: " + name);

    return value;
}

/*----------------------------------------------------------------------------*/
std::string RegistryKey::getString(const std::string& key,
                                   const std::string& valueName)
{
    RegistryKey regKey(key);
    return regKey.asString(valueName);
}

/*----------------------------------------------------------------------------*/
DWORD RegistryKey::getDword(const std::string& key,
                            const std::string& valueName)
{
    RegistryKey regKey(key);
    return regKey.asDword(valueName);
}

/*----------------------------------------------------------------------------*/


/* eof */

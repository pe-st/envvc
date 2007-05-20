/**
 * @file
 * @brief Umgebung f�r Visual Studio setzen
 *
 *         $Id: //depot.hugwi.ch/master/Tools/misc/envvc.cpp#3 $
 *     $Change: 21153 $
 *   $DateTime: 2005/05/02 15:19:12 $
 *     $Author: peter.steiner $
 * $Maintainer: peter.steiner $
 *    $Created: peter.steiner 2005/04/07 $
 *  $Copyright: Hug-Witschi AG, CH-3178 B�singen, http://www.hugwi.ch $
 *
 */

/* Header f�r dieses Modul ---------------------------------------------------*/

/* Standard-Header (ANSI- bzw. HW-Standard) ----------------------------------*/
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>    // for std::runtime_error
#include <stdlib.h>     // getenv, _putenv
#include <process.h>    // _spawnvp

/* Header von anderen Modulen ------------------------------------------------*/
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
|   lokale Konstanten und Makros                                               |
+-----------------------------------------------------------------------------*/

const string msDir("HKLM\\SOFTWARE\\Microsoft\\");
const string studioDir("HKLM\\SOFTWARE\\Microsoft\\VisualStudio\\");
const string expressDir("HKLM\\SOFTWARE\\Microsoft\\VCExpress\\");

const string banner("envvc - environment tool for Visual C++ X.Y\n"
                    "    (c) 2005 Hug-Witschi AG\n");

/*-----------------------------------------------------------------------------+
|   lokale Typen                                                               |
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
|   Deklaration von lokalen Funktionen (Hilfsfunktionen)                       |
+-----------------------------------------------------------------------------*/

static void printUsage();
static void doVC6();
static void doVC71();
static void doVC80();

static std::string trimmedString(const std::string& key,
                                 const std::string& valueName);

static std::string getEnv(const std::string& var);
static void putEnv(const std::string& var, const std::string& value);

/*-----------------------------------------------------------------------------+
|   Modul-globale Variablen                                                    |
+-----------------------------------------------------------------------------*/

string envCollection;
string compiler;

/*-----------------------------------------------------------------------------+
|   Funktionen                                                                 |
+-----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*/
int main (int argc, char* argv[])
{
    int retval = 1;
    try {
        bool isVerbose = false;
        if (argc > 1)
        {
            string arg1(argv[1]);
            if (arg1 == "-v")
            {
                isVerbose = true;
                --argc;
                ++argv;
            }
        }

        if (argc <= 1)
        {
            printUsage();
            exit(1);
        }

        string version(argv[1]);
        if (version == "6" || version == "60")
            doVC6();
        else if (version == "71")
            doVC71();
        else if (version == "80")
            doVC80();
        else
        {
            printUsage();
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
|   Lokale Funktionen (Hilfsfunktionen)                                        |
+-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static void printUsage()
{
    cout << banner
         << "    usage: envvc [-v] 6|60|71|80 [command]\n"
         << endl;
}

/*----------------------------------------------------------------------------*/
static void doVC6()
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
    }
}

/*----------------------------------------------------------------------------*/
static void doVC71()
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
}

/*----------------------------------------------------------------------------*/
static void doVC80()
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

    // these are taken from
    // "C:\Programme\Microsoft Visual Studio 8\Common7\Tools\vsvars32.bat"
    putEnv("VSINSTALLDIR", vsDir);
    putEnv("VCINSTALLDIR", vc8);
    putEnv("FrameworkDir", clrRoot);
    putEnv("FrameworkVersion", clrVers);
    putEnv("FrameworkSDKDir", clrSdk);
    putEnv("DevEnvDir", ideDir);

    string oldpath = getEnv("PATH");
    string oldinc = getEnv("INCLUDE");
    string oldlib = getEnv("LIB");

    string newpath
        = ideDir + ";"
        + vc8 + "\\bin;"
        + vc8 + "\\vcpackages;"
        + common7 + "\\tools;"
        + common7 + "\\tools\\bin;"
        + clrSdk + "\\bin;"
        + clrRoot + "\\" + clrVers + ";"
        + oldpath;
    string newinc
        = vc8 + "\\atlmfc\\include;"
        + vc8 + "\\include;"
        + vc8 + "\\platformSDK\\include;"
        + clrSdk + "\\include;"
        + oldinc;
    string newlib
        = vc8 + "\\atlmfc\\lib;"
        + vc8 + "\\lib;"
        + vc8 + "\\platformSDK\\lib;"
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
|   Environment functions                                                      |
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

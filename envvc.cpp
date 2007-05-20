/**
 * @file
 * @brief Umgebung für Visual Studio setzen
 *
 *         $Id: //depot.hugwi.ch/master/Tools/misc/envvc.cpp#1 $
 *     $Change: 21002 $
 *   $DateTime: 2005/04/13 14:01:22 $
 *     $Author: peter.steiner $
 * $Maintainer: peter.steiner $
 *    $Created: peter.steiner 2005/04/07 $
 *  $Copyright: Hug-Witschi AG, CH-3178 Bösingen, http://www.hugwi.ch $
 *
 */

/* Header für dieses Modul ---------------------------------------------------*/

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

const string msDir("SOFTWARE\\Microsoft\\");
const string studioDir("SOFTWARE\\Microsoft\\VisualStudio\\");


/*-----------------------------------------------------------------------------+
|   lokale Typen                                                               |
+-----------------------------------------------------------------------------*/


class RegistryKey {
public:
    class Exc {};   /// exception class

    explicit RegistryKey(const std::string& key);
    ~RegistryKey();

    std::string asString(const std::string& name) const;

    static std::string getString(const std::string& key,
                                 const std::string& valueName);
private:
    HKEY keyHandle_;
};


/*-----------------------------------------------------------------------------+
|   Deklaration von lokalen Funktionen (Hilfsfunktionen)                       |
+-----------------------------------------------------------------------------*/

static void printUsage();
static int doVC6(int argc, char* argv[]);
static int doVC71(int argc, char* argv[]);
static int doVC80(int argc, char* argv[]);

static std::string trimmedString(const std::string& key,
                                 const std::string& valueName);

static std::string getEnv(const std::string& var);
static void putEnv(const std::string& var, const std::string& value);

/*-----------------------------------------------------------------------------+
|   Modul-globale Variablen                                                    |
+-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------+
|   Funktionen                                                                 |
+-----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*/
int main (int argc, char* argv[])
{
    if (argc <= 2)
    {
        printUsage();
        exit(1);
    }

    int retval = 1;
    try {
        string version(argv[1]);
        if (version == "6" || version == "60")
            retval = doVC6(argc-2, argv+2);
        else if (version == "71")
            retval = doVC71(argc-2, argv+2);
        else if (version == "80")
            retval = doVC80(argc-2, argv+2);
        else
        {
            printUsage();
            exit(1);
        }

        if (retval == -1)
        {
            cout << "failed to execute " << argv[2] << ": errno " << errno << ", \""
                 << strerror(errno) << "\"\n";
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
    cout <<
        "envvc - environment tool for Visual C++ X.Y\n"
        "    (c) 2005 Hug-Witschi AG\n"
        "    usage: envvc 6|71|80 command\n"
         << endl;
}

/*----------------------------------------------------------------------------*/
static int doVC6(int argc, char* argv[])
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

    return static_cast<int>(_spawnvp(_P_WAIT, argv[0], argv));
}

/*----------------------------------------------------------------------------*/
static int doVC71(int argc, char* argv[])
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

    return static_cast<int>(_spawnvp(_P_WAIT, argv[0], argv));
}

/*----------------------------------------------------------------------------*/
static int doVC80(int argc, char* argv[])
{
    string vc8     = trimmedString(studioDir + "8.0\\Setup\\VC",
                                   "ProductDir");
    string vsDir   = trimmedString(studioDir + "8.0\\Setup\\VS",
                                   "ProductDir");
    string common7 = trimmedString(studioDir + "8.0\\Setup\\VS",
                                   "VS7CommonDir");
    string ideDir  = trimmedString(studioDir + "8.0\\Setup\\VS",
                                   "EnvironmentDirectory");
    string clrVers = trimmedString(studioDir + "8.0",
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

    putEnv("LIBPATH", clrRoot + clrVers);

    // these are new, but needed for v86.mak
    putEnv("VC_VERS", "80");

    return static_cast<int>(_spawnvp(_P_WAIT, argv[0], argv));
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
}

/*----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------+
|   RegistryKey methods                                                        |
+-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
RegistryKey::RegistryKey(const std::string& key)
    : keyHandle_(0)
{
    // for now always HKLM
    HKEY hkey = HKEY_LOCAL_MACHINE;

    LONG result = RegOpenKeyEx(hkey,
                               key.c_str(),
                               NULL,
                               KEY_READ, // read access is enough for now
                               &keyHandle_);

    if (result != ERROR_SUCCESS || keyHandle_ == 0)
        throw RegistryKey::Exc();
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
        throw RegistryKey::Exc();
    if (type != REG_SZ && type != REG_EXPAND_SZ)
        throw RegistryKey::Exc();

    // now get the real data
    vector<char> buffer(size);
    result = RegQueryValueEx(keyHandle_,
                             name.c_str(), NULL,
                             &type,
                             reinterpret_cast<LPBYTE>(&buffer[0]),
                             &size);

    if (result != ERROR_SUCCESS)
        throw RegistryKey::Exc();

    if (type == REG_SZ)
    {
        // don't keep trailing zeroes
        vector<char>::iterator pos;
        pos = std::find(buffer.begin(), buffer.end(), '\0');
        return string(buffer.begin(), pos);
    }
    else if (type == REG_SZ)
    {
        // use ExpandEnvironmentStrings here?
        return string(buffer.begin(), buffer.end());
    }
    else
        throw RegistryKey::Exc();
}

/*----------------------------------------------------------------------------*/
std::string RegistryKey::getString(const std::string& key,
                                   const std::string& valueName)
{
    RegistryKey regKey(key);
    return regKey.asString(valueName);
}

/*----------------------------------------------------------------------------*/


/* eof */

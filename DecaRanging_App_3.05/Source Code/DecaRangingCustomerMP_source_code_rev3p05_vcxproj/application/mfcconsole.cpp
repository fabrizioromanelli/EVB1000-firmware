/* Credit for this goes to http://homepage3.nifty.com/ysflight/mfcconsole/mfcconsole.html#VC6

Add this file "mfcconsole.cpp" to the eproject. Sometimes, Visual Studio apparently exclude
the included file from build for unknown reasons.  To check if the file is excluded from build,
click "mfcconsole.cpp" in "Solution Explorer" window, and choose "Property." Then, make sure
"Exclude from Build" is set to "No."  If not, select "No" from the drop list.


(2) In "Solution Explorer" window, choose the project name by mouse left button.
(3) Choose "Project" menu -> "Property", and choose "Linker" -> "System" in the property dialog.
    Then, set "SubSystem" to "Console (/SUBSYSTEM:CONSOLE)".  (Choose from the drop list.)


This is it.  Now you can compile and run the program to see your MFC application with a console window.
When you need to remove the console window, you can simply set this "SubSystem" back to "Windows (/SUBSYSTEM:WINDOWS)."

*/

#include "stdafx.h"

#include <stdio.h>
#include <windows.h>

extern "C"
{

// int PASCAL WinMain(HINSTANCE inst,HINSTANCE dumb,LPSTR param,int show);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow) ;

};

int main(int ac,char *av[])
{
    char buf[256];
    int i;
    HINSTANCE inst;

    inst=(HINSTANCE)GetModuleHandle(NULL);

    buf[0]=0;
    for(i=1; i<ac; i++)
    {
        strcat_s(buf,sizeof(buf),av[i]);
        strcat_s(buf,sizeof(buf)," ");
    }

//  return WinMain(inst,NULL,buf,SW_SHOWNORMAL);
    return _tWinMain(inst,NULL,(LPTSTR)buf,SW_SHOWNORMAL);
}


#include "configfile.h"

extern bool on_top;
extern bool usingball;
extern bool clickMouse;
extern bool lefty;
extern bool club_lockstep;
extern bool driving_range;
extern bool front_sensor_features;
extern bool arcade_mode;

extern HWND mainWindow;
extern HWND clubSelect;
extern HWND reswingButton;
extern HWND clickmouseValue;
extern HWND usingballValue;
extern HWND clubSpeedValue;
extern HWND faceAngleValue;
extern HWND pathValue;
extern HWND faceContactValue;
extern HWND arcadeValue;
extern HWND arcadeUp;
extern HWND arcadeDown;

extern Node* clubs;
extern Node* current_selected_club;

extern std::string ip_addr;

extern double arcade_mult;

extern int num_clubs;
extern int selected_club_value;

void readconfig(HWND hWnd) {

    // std::ifstream is RAII, i.e. no need to call close
    std::ifstream cFile("config.ini");
    if (cFile.is_open())
    {
        std::string line;
        while (getline(cFile, line))
        {
            line.erase(std::remove_if(line.begin(), line.end(), isspace),
                line.end());
            if (line.empty() || line[0] == '#')
            {
                continue;
            }
            auto delimiterPos = line.find("=");
            auto name = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);

            if (!(name.compare("Ball") || value.compare("False"))) {
                usingball = FALSE;
                Button_SetCheck(usingballValue, BST_UNCHECKED);
            }
            else if (!(name.compare("Shot") || value.compare("True"))) {
                clickMouse = TRUE;
                Button_SetCheck(clickmouseValue, BST_CHECKED);
            }
            else if (!(name.compare("Top") || value.compare("True"))) {
                on_top = TRUE;
                SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                HMENU hmenu = GetMenu(hWnd);
                MENUITEMINFO menuItem = { 0 };
                menuItem.cbSize = sizeof(MENUITEMINFO);
                menuItem.fMask = MIIM_STATE;
                GetMenuItemInfo(hmenu, ID_FILE_KEEPONTOP, FALSE, &menuItem);
                menuItem.fState = MFS_CHECKED;
                SetMenuItemInfo(hmenu, ID_FILE_KEEPONTOP, FALSE, &menuItem);
            }
            else if (!(name.compare("Lefty") || value.compare("True"))) {
                lefty = TRUE;
                HMENU hmenu = GetMenu(hWnd);
                MENUITEMINFO menuItem = { 0 };
                menuItem.cbSize = sizeof(MENUITEMINFO);
                menuItem.fMask = MIIM_STATE;
                GetMenuItemInfo(hmenu, ID_FILE_LEFTHANDMODE, FALSE, &menuItem);
                menuItem.fState = MFS_CHECKED;
                SetMenuItemInfo(hmenu, ID_FILE_LEFTHANDMODE, FALSE, &menuItem);
            }
            else if (!(name.compare("Lockstep") || value.compare("True"))) {
                club_lockstep = TRUE;
                HMENU hmenu = GetMenu(hWnd);
                MENUITEMINFO menuItem = { 0 };
                menuItem.cbSize = sizeof(MENUITEMINFO);
                menuItem.fMask = MIIM_STATE;
                GetMenuItemInfo(hmenu, ID_OPTIONS_LOCKSTEPMODE, FALSE, &menuItem);
                menuItem.fState = MFS_CHECKED;
                SetMenuItemInfo(hmenu, ID_OPTIONS_LOCKSTEPMODE, FALSE, &menuItem);
            }
            else if (!(name.compare("Arcade") || value.compare("True"))) {
                arcade_mode = TRUE;
                HMENU hmenu = GetMenu(hWnd);
                MENUITEMINFO menuItem = { 0 };
                menuItem.cbSize = sizeof(MENUITEMINFO);
                menuItem.fMask = MIIM_STATE;
                GetMenuItemInfo(hmenu, ID_OPTIONS_ARCADEMODE, FALSE, &menuItem);
                menuItem.fState = MFS_CHECKED;
                SetMenuItemInfo(hmenu, ID_OPTIONS_ARCADEMODE, FALSE, &menuItem);

                ShowWindow(arcadeUp, SW_NORMAL);
                ShowWindow(arcadeDown, SW_NORMAL);
                ShowWindow(arcadeValue, SW_NORMAL);
            }
            else if (!(name.compare("ArcadeScale"))) {
                arcade_mult = stod(value);
                std::wstringstream wss;
                wss << arcade_mult;
                SetWindowText(arcadeValue, wss.str().c_str());
            }
            else if (!(name.compare("DrivingRange") || value.compare("True"))) {
                driving_range = TRUE;
                HMENU hmenu = GetMenu(hWnd);
                MENUITEMINFO menuItem = { 0 };
                menuItem.cbSize = sizeof(MENUITEMINFO);
                menuItem.fMask = MIIM_STATE;
                GetMenuItemInfo(hmenu, ID_OPTIONS_DRIVINGRANGEMODE, FALSE, &menuItem);
                menuItem.fState = MFS_CHECKED;
                SetMenuItemInfo(hmenu, ID_OPTIONS_DRIVINGRANGEMODE, FALSE, &menuItem);
            }
            else if (!(name.compare("FrontSensor") || value.compare("True"))) {
                front_sensor_features = TRUE;
                HMENU hmenu = GetMenu(hWnd);
                MENUITEMINFO menuItem = { 0 };
                menuItem.cbSize = sizeof(MENUITEMINFO);
                menuItem.fMask = MIIM_STATE;
                GetMenuItemInfo(hmenu, ID_OPTIONS_FRONTSENSORFEATURES, FALSE, &menuItem);
                menuItem.fState = MFS_CHECKED;
                SetMenuItemInfo(hmenu, ID_OPTIONS_FRONTSENSORFEATURES, FALSE, &menuItem);
            }
            else if (!(name.compare("IP"))) {
                ip_addr = value;
            }
            else if (!(name.compare("Driver") || value.compare("True"))) {
                insertNode(&clubs, Driver);
            }
            else if (!(name.compare("2Wood") || value.compare("True"))) {
                insertNode(&clubs, W2);
            }
            else if (!(name.compare("3Wood") || value.compare("True"))) {
                insertNode(&clubs, W3);
            }
            else if (!(name.compare("4Wood") || value.compare("True"))) {
                insertNode(&clubs, W4);
            }
            else if (!(name.compare("5Wood") || value.compare("True"))) {
                insertNode(&clubs, W5);
            }
            else if (!(name.compare("2Hybrid") || value.compare("True"))) {
                insertNode(&clubs, HY2);
            }
            else if (!(name.compare("3Hybrid") || value.compare("True"))) {
                insertNode(&clubs, HY3);
            }
            else if (!(name.compare("4Hybrid") || value.compare("True"))) {
                insertNode(&clubs, HY4);
            }
            else if (!(name.compare("5Hybrid") || value.compare("True"))) {
                insertNode(&clubs, HY5);
            }
            else if (!(name.compare("2Iron") || value.compare("True"))) {
                insertNode(&clubs, I2);
            }
            else if (!(name.compare("3Iron") || value.compare("True"))) {
                insertNode(&clubs, I3);
            }
            else if (!(name.compare("4Iron") || value.compare("True"))) {
                insertNode(&clubs, I4);
            }
            else if (!(name.compare("5Iron") || value.compare("True"))) {
                insertNode(&clubs, I5);
            }
            else if (!(name.compare("6Iron") || value.compare("True"))) {
                insertNode(&clubs, I6);
            }
            else if (!(name.compare("7Iron") || value.compare("True"))) {
                insertNode(&clubs, I7);
            }
            else if (!(name.compare("8Iron") || value.compare("True"))) {
                insertNode(&clubs, I8);
            }
            else if (!(name.compare("9Iron") || value.compare("True"))) {
                insertNode(&clubs, I9);
            }
            else if (!(name.compare("Pitch") || value.compare("True"))) {
                insertNode(&clubs, PW);
            }
            else if (!(name.compare("Gap") || value.compare("True"))) {
                insertNode(&clubs, GW);
            }
            else if (!(name.compare("Sand") || value.compare("True"))) {
                insertNode(&clubs, SW);
            }
            else if (!(name.compare("Lob") || value.compare("True"))) {
                insertNode(&clubs, LW);
            }
            else if (!(name.compare("Putter") || value.compare("True"))) {
                insertNode(&clubs, Putter);
            }
        }

        current_selected_club = clubs;

        SendMessage(clubSelect, CB_RESETCONTENT, 0, 0);

        TCHAR A[16];
        int  k = 0;

        Node* currentclub = clubs;
        club clubToAdd = getClubData(currentclub->club);
        selected_club_value = currentclub->club;

        memset(&A, 0, sizeof(A));
        wcscpy_s(A, sizeof(A) / sizeof(TCHAR), clubToAdd.name);

        // Add string to combobox.
        SendMessage(clubSelect, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
        num_clubs++;

        currentclub = currentclub->next;

        while (currentclub->club != clubs->club)
        {
            clubToAdd = getClubData(currentclub->club);

            wcscpy_s(A, sizeof(A) / sizeof(TCHAR), clubToAdd.name);

            // Add string to combobox.
            SendMessage(clubSelect, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
            num_clubs++;

            currentclub = currentclub->next;
        }

        // Send the CB_SETCURSEL message to display an initial item 
        //  in the selection field  
        SendMessage(clubSelect, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    }
    else {
        insertNode(&clubs, Driver);
        insertNode(&clubs, W3);
        insertNode(&clubs, W5);
        insertNode(&clubs, HY5);
        insertNode(&clubs, I5);
        insertNode(&clubs, I6);
        insertNode(&clubs, I7);
        insertNode(&clubs, I8);
        insertNode(&clubs, I9);
        insertNode(&clubs, PW);
        insertNode(&clubs, GW);
        insertNode(&clubs, SW);
        insertNode(&clubs, LW);
        insertNode(&clubs, Putter);
        num_clubs = 13;
        current_selected_club = clubs;
    }

    RedrawWindow(mainWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}

void saveconfig() {
    std::ofstream cfgfile;

    cfgfile.open("config.ini");
    //take shot
    if (clickMouse) cfgfile << "Shot=True\n";
    else cfgfile << "Shot=False\n";
    //use ball
    if (usingball) cfgfile << "Ball=True\n";
    else cfgfile << "Ball=False\n";
    //on top
    if (on_top) cfgfile << "Top=True\n";
    else cfgfile << "Top=False\n";
    //lefty
    if (lefty) cfgfile << "Lefty=True\n";
    else cfgfile << "Lefty=False\n";
    //lockstep
    if (club_lockstep) cfgfile << "Lockstep=True\n";
    else cfgfile << "Lockstep=False\n";
    //arcademode
    if (arcade_mode) cfgfile << "Arcade=True\n";
    else cfgfile << "Arcade=False\n";
    //arcademult
    cfgfile << "ArcadeScale=" << arcade_mult << "\n";
    //drivingrange
    if (driving_range) cfgfile << "DrivingRange=True\n";
    else cfgfile << "DrivingRange=False\n";
    //frontsensor
    if (front_sensor_features) cfgfile << "FrontSensor=True\n";
    else cfgfile << "FrontSensor=False\n";
    //host ip addr
    cfgfile << "IP=" << ip_addr << "\n";
    //clubs
    cfgfile << "Driver=True\n";
    cfgfile << "2Wood=False\n";
    cfgfile << "3Wood=True\n";
    cfgfile << "4Wood=False\n";
    cfgfile << "5Wood=True\n";
    cfgfile << "2Hybrid=False\n";
    cfgfile << "3Hybrid=False\n";
    cfgfile << "4Hybrid=False\n";
    cfgfile << "5Hybrid=True\n";
    cfgfile << "2Iron=False\n";
    cfgfile << "3Iron=False\n";
    cfgfile << "4Iron=False\n";
    cfgfile << "5Iron=True\n";
    cfgfile << "6Iron=True\n";
    cfgfile << "7Iron=True\n";
    cfgfile << "8Iron=True\n";
    cfgfile << "9Iron=True\n";
    cfgfile << "Pitch=True\n";
    cfgfile << "Gap=True\n";
    cfgfile << "Sand=True\n";
    cfgfile << "Lob=True\n";
    cfgfile << "Putter=True\n";
    cfgfile.close();

}
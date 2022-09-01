#include "shotprocessing.h"

extern bool usingball;
extern bool logging;
extern bool lefty;
extern bool driving_range;
extern bool clickMouse;
extern bool arcade_mode;
extern bool club_lockstep;

extern int desktop_width;
extern int desktop_height;
extern double swing_angle;
extern int swing_path;
extern int face_contact;
extern double backswingstepsize;
extern double backswingsteps;
extern int midswingdelay;
extern double forwardswingstepsize;
extern double forwardswingsteps;
extern double sideaccel;
extern double sidescale;
extern double slope;
extern int selected_club_value;

extern double arcade_mult;

extern HWND mainWindow;
extern HWND clubSpeedValue;
extern HWND faceAngleValue;
extern HWND faceContactValue;
extern HWND pathValue;

extern FILE* fp;

void processShotData(uint8_t* data, int data_size) {

    // FIGURE OUT DATA PROCESSING
    bool first_front = TRUE;
    int elapsed_time = 0;
    bool potential_ball_read = FALSE;
    bool no_ball = FALSE;
    double swing_speed = 0.0;
    int min_front = 7;
    int max_front = 0;
    int min_back = 7;
    int max_back = 0;
    double front_average = 0.0;
    int front_count = 0;
    double back_average = 0.0;
    int back_count = 0;
    int ball_skip = 0;

    for (int i = 0; i < data_size; i += 5) {
        if (data[i + 2] == 0x81) {
            printf("Origin:     Sensor(s) ");
            if (data[i] != 0) {
                for (int j = 0; j < 8; j++) {
                    if (((data[i] >> j) & 0x01) != 0) {
                        if (j < min_front) min_front = j;
                        if (j > max_front) max_front = j;
                        front_average += j;
                        front_count++;
                    }
                    if (((data[i] >> j) & 0x01) != 0) printf("F%d, ", j);
                    else printf("    ");
                }
            }
            else {
                for (int j = 0; j < 8; j++) {
                    if (((data[i + 1] >> j) & 0x01) != 0) {
                        if (j < min_back) min_back = j;
                        if (j > max_back) max_back = j;
                        back_average += j;
                        back_count++;
                    }
                    if (((data[i + 1] >> j) & 0x01) != 0) printf("B%d, ", j);
                    else printf("    ");
                }
            }
            printf("\n");
        }
        if (data[i + 2] == 0x52) {
            elapsed_time += ((data[i + 3] * 256) + data[i + 4]);
            printf("Additional: Sensor(s) ");
            for (int j = 0; j < 8; j++) {
                if (((data[i] >> j) & 0x01) != 0) printf("ERROR IN UNDERSTANDING ");
            }
            for (int j = 0; j < 8; j++) {
                if (((data[i + 1] >> j) & 0x01) != 0) {
                    if (j < min_back) min_back = j;
                    if (j > max_back) max_back = j;
                    back_average += j;
                    back_count++;
                }
                if (((data[i + 1] >> j) & 0x01) != 0) printf("B%d, ", j);
                else printf("    ");
            }
            printf("\n");
        }
        if (data[i + 2] == 0x4A) {
            elapsed_time += ((data[i + 3] * 256) + data[i + 4]);
            printf("Additional: Sensor(s) ");
            for (int j = 0; j < 8; j++) {
                if (((data[i] >> j) & 0x01) != 0) {
                    if (j < min_front) min_front = j;
                    if (j > max_front) max_front = j;
                    front_average += j;
                    front_count++;
                }
                if (((data[i] >> j) & 0x01) != 0) printf("F%d, ", j);
                else printf("    ");
            }
            for (int j = 0; j < 8; j++) {
                if (((data[i + 1] >> j) & 0x01) != 0) printf("ERROR IN UNDERSTANDING ");
            }
            if (first_front) {
                first_front = FALSE;
                //printf("Elapsed: %d, Gap: %d\n", elapsed_time, ((data[i + 3] * 256) + data[i + 4]));
                swing_speed = (double(SENSORSPACING) / double(elapsed_time * 18)) * 2236.94;
            }
            else if (usingball) {
                if (!no_ball) {
                    if (potential_ball_read) {
                        if (((data[i + 3] * 256) + data[i + 4]) < 0x20) {
                            swing_speed = (double(SENSORSPACING) / (double(elapsed_time - ((data[i - 2] * 256) + data[i - 1])) * 18)) * 2236.94;
                            ball_skip = i - 5;
                            printf("Ball detected, swing speed updated!\n");
                        }
                        else {
                            no_ball = TRUE;
                        }
                    }
                    if (((data[i + 3] * 256) + data[i + 4]) > 0x25) {
                        potential_ball_read = TRUE;
                        printf("Maybe a ball?\n");
                    }
                }
            }
            printf("\n");
        }
    }

    //data hex printout
    for (int i = 0; i < data_size; i++) {
        if (i % 20 == 0 && i != 0) printf("\n");
        else if (i % 5 == 0 && i != 0) printf("  ");
        printf("%02hhx ", data[i]);
        if (logging) fprintf(fp, "%d,", data[i]);
    }
    printf("\n\n");
    if (logging) fprintf(fp, "\n");


    std::wstringstream clubspeed;
    //Swing Speed MPH
    if (!first_front) clubspeed << std::fixed << std::setw(3) << std::setprecision(1) << swing_speed << " MPH\n";
    SetWindowText(clubSpeedValue, clubspeed.str().c_str());

    //Club Open/Closed
    int prev_min, prev_max, min, max;
    double min_angle, max_angle;
    double back_angle_accumulate = 0.0;
    int back_angle_count = 0;
    double front_angle_accumulate = 0.0;
    int front_angle_count = 0;
    bool front = FALSE;
    double speed_per_tick = double(SENSORSPACING) / double(elapsed_time);
    for (int i = 0; i < data_size; i += 5) {
        if (i == 0) {
            int temp_data = 0;
            if (data[i] == 0) temp_data = data[i + 1];
            else temp_data = data[i];
            int j = 0;
            while (((temp_data >> j) & 0x1) == 0) j++;
            prev_min = j;
            j = 7;
            while (((temp_data >> j) & 0x1) == 0) j--;
            prev_max = j;
        }
        else {
            if (i >= ball_skip) {
                if (data[i] != 0 && !front) {
                    int temp_data = 0;
                    if (data[i] == 0) temp_data = data[i + 1];
                    else temp_data = data[i];
                    int j = 0;
                    while (((temp_data >> j) & 0x1) == 0) j++;
                    prev_min = j;
                    j = 7;
                    while (((temp_data >> j) & 0x1) == 0) j--;
                    prev_max = j;
                    front = TRUE;
                }
                else {
                    if (data[i] != 00 || data[i + 1] != 00) {
                        int temp_data = 0;
                        if (data[i] == 0) temp_data = data[i + 1];
                        else temp_data = data[i];
                        int j = 0;
                        while (((temp_data >> j) & 0x1) == 0) j++;
                        min = j;
                        j = 7;
                        while (((temp_data >> j) & 0x1) == 0) j--;
                        max = j;
                        double x_travel = speed_per_tick * ((data[i + 3] * 256) + data[i + 4]);
                        int y_travel_min = (prev_min - min) * LEDSPACING;
                        if (y_travel_min == 0) y_travel_min = 1000000;
                        int y_travel_max = (prev_max - max) * LEDSPACING;
                        if (y_travel_max == 0) y_travel_max = 1000000;
                        //printf("x: %f, Ymin: %d, Ymax: %d\n", x_travel, y_travel_min, y_travel_max);
                        min_angle = atan(x_travel / y_travel_min) * 180 / PI;
                        max_angle = atan(x_travel / y_travel_max) * 180 / PI;

                        if (front) {
                            //printf("Min: %f, Max %f\n", min_angle, max_angle);
                            if (abs(min_angle) > abs(max_angle)) front_angle_accumulate += min_angle;
                            else front_angle_accumulate += max_angle;
                            front_angle_count++;
                        }
                        else {
                            //printf("Min: %f, Max %f\n", min_angle, max_angle);
                            if (abs(min_angle) > abs(max_angle)) back_angle_accumulate += min_angle;
                            else back_angle_accumulate += max_angle;
                            back_angle_count++;
                        }
                        prev_min = min;
                        prev_max = max;
                    }
                }
            }
        }
    }
    //printf("%f, %f, %d, %d\n", front_angle_accumulate, back_angle_accumulate, front_angle_count, back_angle_count);
    double average = ((front_angle_accumulate / front_angle_count) + (back_angle_accumulate / back_angle_count) * 2) / 3; //weighted as the ball is closer to the back sensor
    if (isnan(average)) average = 0.0;
    //printf("Front: %f, Back %f\n", front_angle_accumulate / front_angle_count, back_angle_accumulate / back_angle_count);
    //wss << "Face Angle: ";
    std::wstringstream faceangle;
    faceangle << std::fixed << std::setw(3) << std::setprecision(1);
    if (lefty) {
        if (average > 0) faceangle << "Open " << average << "\n";
        else if (average < 0) faceangle << "Closed " << abs(average) << "\n";
        else faceangle << "Square " << abs(average) << "\n";
    }
    else {
        if (average > 0) faceangle << "Closed " << average << "\n";
        else if (average < 0) faceangle << "Open " << abs(average) << "\n";
        else faceangle << "Square " << abs(average) << "\n";
    }
    SetWindowText(faceAngleValue, faceangle.str().c_str());
    swing_angle = average;

    //Club Path
    int max_path = max_front - max_back;
    int min_path = min_front - min_back;
    int path = max_path + min_path;
    //wss << "max: " << max_path << " min: " << min_path << " path: " << path << "\n";
    //printf("%d ", path);
    //wss << "Path: ";
    std::wstringstream clubpath;
    if (lefty) {
        if (abs(path) > 3) {
            if (path > 0) clubpath << "Very Outside/In\n";
            else clubpath << "Very Inside/Out\n";
        }
        else if (abs(path) > 1) {
            if (path > 0) clubpath << "Outside/In\n";
            else clubpath << "Inside/Out\n";
        }
        else clubpath << "On Plane\n";
    }
    else {
        if (abs(path) > 3) {
            if (path > 0) clubpath << "Very Inside/Out\n";
            else clubpath << "Very Outside/In\n";
        }
        else if (abs(path) > 1) {
            if (path > 0) clubpath << "Inside/Out\n";
            else clubpath << "Outside/In\n";
        }
        else clubpath << "On Plane\n";
    }
    SetWindowText(pathValue, clubpath.str().c_str());
    swing_path = path;

    //Club Face Hit Location
    //wss << "Face Contact: ";
    std::wstringstream facecontact;
    //Only using back, matches OS software better, and makes more sense as the front is way after the ball
    int max_trigger = max_back; // max(max_front, max_back);
    int min_trigger = min_back; // max(min_front, min_back);
    if (lefty) {
        if (min_trigger == 7) { facecontact << "Missed\n"; face_contact = 3; }
        else if (min_trigger == 6) { facecontact << "Extreme Toe\n";  face_contact = 2; }
        else if (min_trigger == 5) { facecontact << "Extreme Toe\n";  face_contact = 2; }
        else if (min_trigger == 4) { facecontact << "Toe\n";          face_contact = 1; }
        else if (max_trigger == 0) { facecontact << "Far Heel\n";     face_contact = -2; }
        else if (max_trigger == 1) { facecontact << "Far Heel\n";     face_contact = -2; }
        else if (max_trigger == 2) { facecontact << "Far Heel\n";     face_contact = -2; }
        else if (max_trigger == 3) { facecontact << "Heel\n";         face_contact = -1; }
        else { facecontact << "Center\n"; face_contact = 0; }
    }
    else {
        if (max_trigger == 0) { facecontact << "Missed\n"; face_contact = 3; }
        else if (max_trigger == 1) { facecontact << "Extreme Toe\n";  face_contact = 2; }
        else if (max_trigger == 2) { facecontact << "Extreme Toe\n";  face_contact = 2; }
        else if (max_trigger == 3) { facecontact << "Toe\n";          face_contact = 1; }
        else if (min_trigger == 7) { facecontact << "Far Heel\n";     face_contact = -2; }
        else if (min_trigger == 6) { facecontact << "Far Heel\n";     face_contact = -2; }
        else if (min_trigger == 5) { facecontact << "Far Heel\n";     face_contact = -2; }
        else if (min_trigger == 4) { facecontact << "Heel\n";         face_contact = -1; }
        else { facecontact << "Center\n"; face_contact = 0; }
    }
    SetWindowText(faceContactValue, facecontact.str().c_str());

    //wss << selected_club_value << "\n";
    club current_club = getClubData(selected_club_value);

    if (max_trigger == 0) current_club.smash_factor = 0.5;
    else if (max_trigger == 1) current_club.smash_factor = 0.94;
    else if (max_trigger == 2) current_club.smash_factor = 0.94;
    else if (max_trigger == 3) current_club.smash_factor = 0.98;
    else if (min_trigger == 7) current_club.smash_factor = 0.94;
    else if (min_trigger == 6) current_club.smash_factor = 0.94;
    else if (min_trigger == 5) current_club.smash_factor = 0.94;
    else if (min_trigger == 4) current_club.smash_factor = 0.98;

    //Generate Parameters for shot
    if (selected_club_value == Putter) {
        backswingstepsize = 7;
        forwardswingstepsize = 25;
        if (swing_speed > 5.5) midswingdelay = 100 + int(150 * swing_speed);
        else midswingdelay = 100 + int(100 * swing_speed);
    }
    else
    {
        //The chart this is based on seems to be rather inacurate for it's distances, so reverting to the old code again.
        // 
        //double carry = (swing_speed / 100) * current_club.dist_at_100;
        //
        //if (arcade_mode) carry = carry * arcade_mult;
        //
        //midswingdelay = int(60 + ((carry/current_club.dist_at_100) * 528));
        //if (midswingdelay > 625) midswingdelay = 625; //625 is a full powered shot, so a longer wait won't help.  Sorry if you can outdrive the game :)

        backswingstepsize = 7.0 * current_club.smash_factor;
        forwardswingstepsize = 25.0 * current_club.smash_factor; // 15/50 goes right, 150/7 goes left, no control beyond default

        if (swing_speed > current_club.club_speed) swing_speed = current_club.club_speed; //max out swing speed so you don't delay too long and get a worse shot

        if (selected_club_value > I9) { // Wedges
            midswingdelay = 100 + int(500.0 * (swing_speed / current_club.club_speed));
        }
        else if (selected_club_value > W5) { // Hybrid and Irons
            midswingdelay = 50 + int(575.0 * (swing_speed / current_club.club_speed));
        }
        else { // Driver and Woods
            midswingdelay = 100 + int(525.0 * (swing_speed / current_club.club_speed));
        }

    }

    //If the angle of the club head is too great, TGC doesn't take the shot properly and taps the ball instead of the full swing
    if (average > 17) average = 17;
    if (average < -17) average = -17;
    slope = tan(average * PI / -180) * 4.0; //path of ball is affected by how open/closed the club is

    //sideaccel = (slope - (path * .08333333)) * 10.0; //ball curve is affected by club path relative to face angle
    sideaccel = (path * .08333333) * -20.0; //ball curve is affected by club path relative to face angle

    sidescale = 0;         //Aiming left/right, should be done manually by user

    RedrawWindow(mainWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}

void preciseSleep(double seconds) {
    using namespace std;
    using namespace std::chrono;

    static double estimate = 5e-3;
    static double mean = 5e-3;
    static double m2 = 0;
    static int64_t count = 1;

    while (seconds > estimate) {
        auto start = high_resolution_clock::now();
        this_thread::sleep_for(milliseconds(1));
        auto end = high_resolution_clock::now();

        double observed = (end - start).count() / 1e9;
        seconds -= observed;

        ++count;
        double delta = observed - mean;
        mean += delta / count;
        m2 += delta * (observed - mean);
        double stddev = sqrt(m2 / (count - 1));
        estimate = mean + stddev;
    }

    // spin lock
    auto start = high_resolution_clock::now();
    while ((high_resolution_clock::now() - start).count() / 1e9 < seconds);
}

void setCurve()
{
    INPUT inputs[2] = {};
    ZeroMemory(inputs, sizeof(inputs));

    if (driving_range) {
        inputs[0].type = INPUT_KEYBOARD; //tap W, sometimes the first keyboard input is missed, so this one is throw away
        inputs[0].ki.wVk = 0x57;
        inputs[0].ki.dwFlags = 0;

        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = 0x57;
        inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
        Sleep(200);

        inputs[0].type = INPUT_KEYBOARD; //tap Z to change club
        inputs[0].ki.wVk = 0x5A;
        inputs[0].ki.dwFlags = 0;

        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = 0x5A;
        inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
        Sleep(200);

        inputs[0].type = INPUT_KEYBOARD; //tap x to go back to same club
        inputs[0].ki.wVk = 0x58;         //this resets the shot curve to be straight
        inputs[0].ki.dwFlags = 0;

        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = 0x58;
        inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
        Sleep(200);
    }

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_SHIFT;
    inputs[0].ki.dwFlags = 0;

    if (sideaccel > 0) {
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = VK_RIGHT;
        inputs[1].ki.dwFlags = 0 | KEYEVENTF_EXTENDEDKEY;

        SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
        preciseSleep(abs(sideaccel) / 25.0);

        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_RIGHT;
        inputs[0].ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY;
    }
    else if (sideaccel < 0) {
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = VK_LEFT;
        inputs[1].ki.dwFlags = 0 | KEYEVENTF_EXTENDEDKEY;

        SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
        preciseSleep(abs(sideaccel) / 25.0);

        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_LEFT;
        inputs[0].ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY;
    }

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_SHIFT;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    Sleep(200);
}

void takeShot() {
    bool return_lockstep = club_lockstep;
    club_lockstep = FALSE;

    double cursorX = desktop_width / 2;
    double cursorY = desktop_height / 2;
    int backswingspeed = 6;
    int forwardswingspeed = 5; //lower is faster
    //double local_sideaccel = sideaccel;

    SetCursorPos(cursorX, cursorY);
    if (clickMouse) {
        mouse_event(MOUSEEVENTF_LEFTDOWN, cursorX, cursorY, 0, 0); //click the window to ensure it's in focus before doing keyboard stuff
        Sleep(200);
        mouse_event(MOUSEEVENTF_LEFTUP, cursorX, cursorY, 0, 0);
        Sleep(200);
        if (selected_club_value != Putter) setCurve();
        mouse_event(MOUSEEVENTF_LEFTDOWN, cursorX, cursorY, 0, 0); //click for start of shot
    }
    for (int i = 0; i < backswingsteps; i++) {
        cursorY += backswingstepsize;
        cursorX -= slope * backswingstepsize;
        if (cursorX < 0) cursorX = 0;
        if (cursorX > desktop_width) cursorX = desktop_width;
        if (cursorY < 0) cursorY = 0;
        if (cursorY > desktop_height) cursorY = desktop_height;
        SetCursorPos(cursorX, cursorY);
        preciseSleep(double(backswingspeed) / 1000.0);
    }
    preciseSleep(double(midswingdelay) / 1000.0); //convert ms to s
    for (int i = 0; i < forwardswingsteps; i++) {
        cursorY -= forwardswingstepsize;
        cursorX += slope * forwardswingstepsize;
        //local_sideaccel = pow(local_sideaccel, 1.2);
        //if (local_sideaccel > desktop_width) local_sideaccel = desktop_width;
        //cursorX += int(local_sideaccel * sidescale);
        if (cursorX < 0) cursorX = 0;
        if (cursorX > desktop_width) cursorX = desktop_width;
        if (cursorY < 0) cursorY = 0;
        if (cursorY > desktop_height) cursorY = desktop_height;
        SetCursorPos(cursorX, cursorY);
        preciseSleep(double(forwardswingspeed) / 1000.0);
    }
    mouse_event(MOUSEEVENTF_LEFTUP, cursorX, cursorY, 0, 0);
    SetCursorPos(cursorX, cursorY);

    club_lockstep = return_lockstep;
}

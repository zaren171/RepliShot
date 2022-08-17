# RepliShot
Replishot is a custom application to capture data from an OptiShot 2 golf simulator and convert it into mouse commands for use in TGC2019.  Please consider supporting Replishot here: [Paypal Support Link](https://www.paypal.com/donate/?business=2CYDC37QAFDV8&no_recurring=0&item_name=Thank+you+for+your+support+of+Replishot%21&currency_code=USD)

## Software Overview

This software allows an Optishot to be used with the standard version of [TGC2019](https://store.steampowered.com/app/695290/The_Golf_Club_2019_featuring_PGA_TOUR/) through mouse emulation.  Shot data is captured from the Optishot and then converted to mouse movements to replicate the measured shot in TGC2019.  It is recommended to run TGC2019 in full screen and to keep Replishot in a lower corner of the screen.  Replishot has been tested with TGC2019 running at 1920x1080.

## Replishot Display

A screenshot of Replishot can be seen here:

![Replishot Screenshot](https://github.com/zaren171/RepliShot/blob/master/Replishot_image.PNG)

The "Take Shot" check box selects whether or not the mouse will be clicked during the shot.  While the box is unchecked the mouse will move, but the mouse will not be clicked.  When this box is checked the mouse will be clicked when Replishot moves the mouse, which will cause the shot to be captured in TGC2019.  

If using a ball ensure "Using Ball" is checked (checked by default) as the ball can be captured by the sensor and affect swing data.

The "Reswing" button will repeat the mouse movements for the last captured shot.  If TGC2019 ever misses a shot you can use this button to send it again.  This is mostly for software testing/debug purposes.

There is a dropdown for club selection.  It is important to select the correct club as this affects the calculation for a "maximum power" shot.  See more about club selection and automatic club updates in the Lockstep portion of the Menus and Options section.  You can select the club you are using in the dropdown menu, or scroll through the clubs using the front sensor on the Optishot.  For more information see Front Sensor Special Functions.  

The "Club Speed", "Face Angle", "Path", and "Face Contact" will dispay the measured and calculated values of the club swing data.  The image at the bottom will be udpated to reflect the previously captured shot.  An example is shown in the following image.

![Replishot Options](https://github.com/zaren171/RepliShot/blob/master/Replishot_swing_data.PNG)

There is also an indicator in the bottom right corner to display whether or not Replishot is currently connected to the Optishot pad.  You can reconnect in the File menu if needed.

## Menus and Options

![Replishot Options](https://github.com/zaren171/RepliShot/blob/master/Replishot_options_menu.PNG)

Under the File menu there is an option to Save Config.  This will generate a config.ini file.  This file will hold the state of the check boxes for Take Shot and Useing Ball, as well as hold the state of the items under the Options menu.  In addition, the first time a config.ini file is written it will write out a default set of clubs that match the default bag from TGC2019.  You can edit this set of clubs (open config.ini in a text editor, change true/false for the clubs to match your desired set, but DO NOT change the club names) to match your own club set, but it is recommended you update TGC2019 to match your clubset as well so the same club can be selected in both locations.

Under the Options menu there is an option to keep Replishot on top of other programs.  This is helpful so that you can continue to see Replishot while TGC2019 is running in full screen.

Next in the Options menu there is an option for Lockstep Mode.  In the lockstep mode Replishot uses OCR too attempt to read the club data from the screen to automatically match the club in Replishot to the one being used in TGC2019.  Replishot will read the top right corner of the screen, so make sure the shot setting information (as can be seen in the final image) is in the top right corner of your screen.  When Replishot successfully reads a club it will automatically change the selected club in the drop down menu to match.  Also while in lockstep mode, changing the selected club using the front sensor on the Optishot will also change the club in TGC2019.  As an example, if you swipe the bottom portion of the front sensor the club in Replishow will decrement, and a "Z" will be input to TGC2019 so that the clubs continue to match.  If the OCR in Replishot is not automatically reading the club name and updating the Replishot selection changing the club a few times can help.

The Options menu has an option for left handed mode, which will adjust the display of metrics, etc. so they are correctly reported for left handed swings.

Arcade mode includes a multiplier to the ball's carry distance.  Arcade mode allows users to hit farther (or shorter if set to < 1) instead of trying to recreate the shots actual distance.  This mode will allow anyone to get additional distance out of their clubs, and play TGC more like a video game.  When Arcade mode is activated additional buttons will appear in the interface as seen in the following screen shot in the red box.  The '+' and '-' buttons increase/decrease the distance muliplier in 0.05 increments (5%).

![Replishot Options](https://github.com/zaren171/RepliShot/blob/master/Replishot_arcade_mult.png)

The option for Driving Range Mode is for use when using the TGC driving range.  At the driving range the shot shape (slice/hook) is not cleared between shots.  In driving range mode the club is quickly switched and then switched back before the shot, clearing any previous slice/hook data.  This is unneccesary when playing a round as every shot resets to straight, even if the same club is suggested 2 shots in a row.

The next option is for Front Sensor Features, which are discussed in the next section.  When checked, these features will be enabled and available for use.

The Network Mode option is still in development.  The goal is to allow two copies of Replishot to communicate with eachother over a network.  This would allows a less powerful computer to talk to the Optishot pad and send data to a more powerful computer running TGC 2019.

## Front Sensor Special Functions

![Replishot Options](https://github.com/zaren171/RepliShot/blob/master/Optishot_front_sensor_split.PNG)

The front sensor can be used three special functions in Replishot.  To use these, you must pass a club over ONLY the front sensor.  The sensor has been split into three parts, the top, bottom and middle.

If you move your club over the lower portion (purple) of the front sensor (looking from a RH swing prospective) the club will decrement (i.e. Driver -> Woods -> Irons -> Putter -> Driver).  Moving the club over the upper portion (red) of the front sensor will increment the club (i.e. Putter -> Wedges -> Irons -> Woods -> Driver -> Putter).  The clubs will wrap around from Putter to Driver and vice-versa.  For club selection you can go beyond the purple/red boxes picture.  So long as a sensor within a red or purple box is triggered the club will be changed.

If your club goes through only the middle portion (blue) (must not trigger the red/purple sensor areas on either end) then Replishot will change the shot selection in TGC2019.  This can be very useful in short range situations where TGC likes to give shots with very short maximum ranges, requiring a full swing to go a short distance.

## Replishot Shot Selection

In general Replishot works well with the "Normal" shot selection in TGC.  For close shots, such as chipping, I personally feel that the "Pitch" setting is more accuarte.  Due to TGC's 5% power increments it is hard to get reliable range on short shots taken in "Normal" mode.  The following screen shot shows TGC's shot preview information.  Pressing "C" will change the shot type, which will change the maximum carry yrds shown.  Note that TGC will automatically change this setting, typically giving you a shot type that will require a full swing.

![Shot Settings Screenshot](https://github.com/zaren171/RepliShot/blob/master/TGC_Shot_Settings.PNG)

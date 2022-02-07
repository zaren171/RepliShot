# RepliShot
Replishot is a custom application to capture data from an OptiShot 2 golf simulator and convert it into mouse commands for use in TGC2019.

This software allows an Optishot to be used with the standard version of TGC2019 through mouse emulation.  Shot data is captured from the Optishot and then converted to mouse movements to replicate the measured shot in TGC2019.  It is recommended to run TGC2019 in full screen and to keep Replishot in a lower corner of the screen.  Replishot has been tested with TGC2019 running at 1920x1080.

A screenshot of Replishot can be seen here:

![Replishot Screenshot](https://github.com/zaren171/RepliShot/blob/master/Replishot_image.PNG)

The "Take Shot" check box selects whether or not the mouse will be clicked during the shot.  While the box is unchecked the mouse will move, but the mouse will not be clicked.  When this box is checked the mouse will be clicked when Replishot moves the mouse, which will cause the shot to be captured in TGC2019.  

If using a ball ensure "Using Ball" is checked (checked by default) as the ball can be captured by the sensor and affect swing data.

The "Reswing" button will repeat the mouse movements for the last captured shot.  If TGC2019 ever misses a shot you can use this button to send it again.  This is mostly for software testing/debug purposes.

There is a dropdown for club selection.  It is important to select the correct club as this affects the calculation for a "maximum power" shot.  You can select the club you are using in the dropdown menu, or scroll through the clubs using the front sensor on the Optishot.  If you move your club over the lower half of the front sensor (looking from a RH swing prospective) the club will decrement (i.e. Driver -> Woods -> Irons -> Putter -> Driver).  Moving the club over the upper portion of the front sensor will increment the club (i.e. Putter -> Wedges -> Irons -> Woods -> Driver -> Putter).  The clubs will wrap around from Putter to Driver and vice-versa.

The "Club Speed", "Face Angle", "Path", and "Face Contact" will dispay the measured and calculated values of the club swing data.  The image at the bottom will be udpated to reflect the previously captured shot.

Under the File menu there is an option to keep Replishot on top of other programs.  This is helpful so that you can continue to see Replishot while TGC2019 is running in full screen.

If you like Replishot, and would be willing to support it's development, it would be greatly appreciated! [Paypal Support Link](https://www.paypal.com/donate/?business=2CYDC37QAFDV8&no_recurring=0&item_name=Thank+you+for+your+support+of+Replishot%21&currency_code=USD)

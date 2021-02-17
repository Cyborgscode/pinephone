#!/bin/bash

gcc flashlight.c -o flashlight; 

chmod 755 flashlight
chmod u+s flashlight
rm -f /usr/local/bin/flashlight
mv flashlight /usr/local/bin/

rm -f /usr/share/applications/flashlight.desktop
cp flashlight.desktop /usr/share/applications/

echo "ready"

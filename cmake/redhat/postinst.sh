#!/bin/sh
#
#


cd /opt/2dx/bin
\rm -f 2dx_image
ln -sf ../2dx_image/2dx_image ./
\rm -f 2dx_merge
ln -sf ../2dx_merge/2dx_merge ./
\rm -f 2dx_logbrowser
ln -sf ../2dx_logbrowser/2dx_logbrowser ./
#

chmod -R 755 /opt/2dx

ln -sf /opt/2dx/bin/2dx_merge /usr/bin/2dx

#cp /opt/2dx/resource/2dx.desktop /usr/share/applications/
#cp /opt/2dx/resource/icon.png /usr/share/pixmaps/2dx.png


#
exit 0

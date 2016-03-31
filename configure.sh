#!/bin/sh

#                     brightness (int)  : min=0 max=255 step=1 default=8 value=0
#                       contrast (int)  : min=0 max=255 step=1 default=32 value=37
#                            hue (int)  : min=0 max=255 step=1 default=165 value=143
#             auto_white_balance (bool) : default=0 value=0
#                    red_balance (int)  : min=0 max=255 step=1 default=128 value=128
#                   blue_balance (int)  : min=0 max=255 step=1 default=128 value=128
#                       exposure (int)  : min=0 max=255 step=1 default=255 value=20
#                       autogain (bool) : default=1 value=0
#                      main_gain (int)  : min=0 max=63 step=1 default=20 value=20
#                          hflip (bool) : default=0 value=0
#                          vflip (bool) : default=0 value=0
#                      sharpness (int)  : min=0 max=63 step=1 default=0 value=0

v4l2-ctl --list-ctrls

v4l2-ctl --verbose --set-ctrl=auto_gain=0
v4l2-ctl --verbose --set-ctrl=brightness=0
v4l2-ctl --verbose --set-ctrl=contrast=48
#v4l2-ctl --verbose --set-ctrl=auto_exposure=0
v4l2-ctl --verbose --set-ctrl=auto_white_balance=0
v4l2-ctl --verbose --set-ctrl=exposure=0
v4l2-ctl --verbose --set-ctrl=main_gain=0
v4l2-ctl --verbose --set-ctrl=hflip=1
v4l2-ctl --verbose --set-ctrl=vflip=0
v4l2-ctl --verbose --set-ctrl=light_frequency_filter=0
v4l2-ctl --verbose --set-ctrl=sharpness=0

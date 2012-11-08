#!/bin/bash
# Proper directfb settings depend 
# on your system. This script add
# working settings on my systems.
# You should modify this on your
# system.
# With argument 'nofullscreen' the line
# m_pDfb->SetCooperativeLevel (m_pDfb, DFSCL_FULLSCREEN);
# will skipped. 

case "$1" in
	pi) 
		# Setting for raspberry pi with
		# rasbian os.
		sudo fbset -a -depth 16 
		sudo bin/main --dfb:mode=1024x768 --dfb:pixelformat=RGB16
		;;
	pi32) 
		# Setting for raspberry pi with
		# rasbian os.
		sudo fbset -a -depth 32
		sudo bin/main --dfb:mode=1024x768 --dfb:pixelformat=RGB32
		;;
	igep)
		# Setting for igep board with
		# linaro 12.x os.
		bin/main --dfb:mode=1024x768 --dfb:pixelformat=RGB16
		;;
	laptop)
		# Setting for aspire3020 with
		# ubuntu 12.4 os.
		bin/main nofullscreen 
		;;
	x11)
		bin/main nofullscreen --dfb:system=x11 --dfb:force-windowed
		;;
	*)
		# Default values
		bin/main 
		;;
esac


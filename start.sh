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
		bin/main --dfb:mode=1024x768 --dfb:pixelformat=RGB16
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
		bin/main nofullscreen --system=x11 
	*)
		# Default values
		bin/main 
		;;
esac


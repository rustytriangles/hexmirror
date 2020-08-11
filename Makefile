all:	hexmirror

hexmirror:	hexmirror.o
	c++ -o hexmirror hexmirror.o -lraspicam -lws2811

hexmirror.o:	hexmirror.cpp
	c++ -c hexmirror.cpp

clean:
	rm hexmirror test_leds test_camera *.o




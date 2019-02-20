#pragma once
/*
 * ###################################### Notice ######################################
 * Implementation of input_event in the linux Kernel:
 * ```cpp
 * struct input_event {
 * 	struct timeval time;
 * 	unsigned short type;
 * 	unsigned short code;
 * 	unsigned int value;
 * };
 * ```
 * [ref 1](https://www.kernel.org/doc/Documentation/input/input.txt)
 * [ref 2](https://github.com/torvalds/linux/blob/master/include/uapi/linux/input.h)
 * ```cpp
 * 
 * // The event structure itself
 * // Note that __USE_TIME_BITS64 is defined by libc based on
 * // application's request to use 64 bit time_t.
 * struct input_event {
 * #if (__BITS_PER_LONG != 32 || !defined(__USE_TIME_BITS64)) && !defined(__KERNEL)
 * 	struct timeval time;
 * #define input_event_sec time.tv_sec
 * #define input_event_usec time.tv_usec
 * #else
 * 	__kernel_ulong_t __sec;
 * 	__kernel_ulong_t __usec;
 * #define input_event_sec  __sec
 * #define input_event_usec __usec
 * #endif
 * 	__u16 type;
 * 	__u16 code;
 * 	__s32 value;
 * };
 * ```
 * ####################################################################################
 **/
#include <thread>
namespace Concordia
{
	class EventMgr;
}
namespace si
{
	/**
	 ** Mouse Data
	 **  This information are based on the event_input event
	 ***/
	struct MouseData
	{
		//relativeValue:
		float relX;
		float relY;
		float absX;
		float absY;
		__signed__ int wheel; //__u32
		__signed__ int valueX; //__u32
		__signed__ int valueY; //__u32
	};
	
	struct key_stage
	{
		char key_pressed[512];
		bool key_is_pressed = false;
		bool key_is_down = false;
	};
	
	class Input
	{
	public:
		Input() = default;
		//Class can be only instanciated once!
		Input(const Input& other) = delete;
		Input(Input&& other) = delete;
		Input& operator=(const Input& other) = delete;
		Input& operator=(const Input&& other) = delete;
		~Input();
		/*
		 * Sets the basic values in the background and init the threads
		 * @param amout_of_devices how many events are possible? Max is 32.
		 * @param mouse_events_enabled shall the system send next to the mouseData update a event mousemoved[x,y] & wheelmoved ?
		 * @param screenW screen width
		 * @param screenH screen height
		 **/
		void init(float screenW, float screenH,int amout_of_devices = 4, bool mouse_events_enabled = false);
		/*
		 * cleans up the system: kills the threads
		 **/
		void deinit();
		void listen(Concordia::EventMgr*);
		const MouseData& GetMouse();
		bool isKeyDown(unsigned short key);
		bool isKeyUp(unsigned short key);
		
	private:
		std::thread* deviceThread;
		/*
		 * Max number of events based on the keyboard default is 4
		 * /dev/input/event[n]
		 **/
		int maxNumberofEvents;
		/*
		 * shall the system send next to the mouseData update a event mousemoved[x,y] & wheelmoved ?
		 * 
		 ***/
		bool fireMouseEvents;
		bool canListen;
		//tell the deconstructor if deint was called
		bool cleaned = false;
	};
}
/**
 **
 ** # Research
 ** 
 ** - https://www.kernel.org/doc/html/v4.13/input/event-codes.html
 ** - https://www.kernel.org/doc/Documentation/input/input.txt
 ** - https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
 ** - https://github.com/spotify/linux/blob/master/drivers/input/mousedev.c
 ** - https://www.kernel.org/doc/Documentation/input/event-codes.txt
 **
 ***/

extern si::Input gSimpleInput;

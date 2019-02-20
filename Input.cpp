#include "Input.h"
#include "InputEvents.h"
#include "Concordia/EventMgr.hpp"

#include <linux/input.h>

/**
 ** Low level POSIX functions:
 **  open()
 **  poll()
 **  read()
 ** @see http://man7.org/linux/man-pages/man2/poll.2.html
 ** @see http://pubs.opengroup.org/onlinepubs/7908799/xsh/open.html
 ** @see https://linux.die.net/man/2/read
 ** @see https://stackoverflow.com/questions/29248295/read-hangs-while-watching-for-event0-using-inotify
 **  
 ***/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <stdio.h>
#include <cstdio>

#include <iostream>
#include <unordered_map>
#include <pthread.h>
#include <signal.h>
#include <exception>
#include <ctime>
namespace
{	
	// http://gynvael.coldwind.pl/?id=406
	static bool g_run = true;
	static si::MouseData g_mouseData;
	static si::key_stage g_keyStage;
	
	static float g_screenW;
	static float g_screenH;
	
}
/**
 **
 ** Decleartions:
 **
 ***/
static void check_events(int eventID, Concordia::EventMgr* evm, bool fireEventEvents);
static void check_for_devices(Concordia::EventMgr* evm,int maxNumberOfEvents = 4,bool fireEventEvents = false);
/**
 **
 ** Defintions:
 **
 ***/

/**
 **
 ** 
 ** @detail checks a event if it has something to wrok with. Only Keyboard,Mouse are supported.(yet)
 ** @param int eventID: is the number X of the event document in /dev/input/event[X]
 ** @param Concordia::EventMgr* a pointer to the Concordia EventManager
 ** 
 ** @see http://man7.org/linux/man-pages/man2/poll.2.html
 ** @see http://pubs.opengroup.org/onlinepubs/7908799/xsh/open.html
 ** @see https://linux.die.net/man/2/read
 ** @see https://stackoverflow.com/questions/29248295/read-hangs-while-watching-for-event0-using-inotify
 ** 
 ** **IMPORTANT** Dont mix low level stream reading with "highlevel" reading such as fread....
 ***/
static void check_events(int eventID, Concordia::EventMgr* evm, bool fireEventEvents /* default of check_devices is false*/)
{
	FILE* ptr_event;
	std::string event_n = "/dev/input/event";
	event_n += std::to_string(eventID);
	//ptr_event = fopen(event_n.c_str(), "r");
	
	// see for O_* https://www.gnu.org/software/libc/manual/html_node/Open_002dtime-Flags.html
	int fd = open(event_n.c_str(), O_RDONLY | O_NONBLOCK); 
	pollfd pfd; // see man 2 poll
	pfd.fd = fd;
	pfd.events = POLLIN;
	auto epollFd = epoll_create(1);
	epoll_event ep_event;
	std::cout << "spwan check event thread \n";
	if (fd != -1)
	{
		struct input_event ev;
		size_t bytes_read;
		while (g_run)
		{	
			
			// int ep = epoll_wait(epollFd, &ep_event, int maxevents, int timeout);
			//struct epoll_event pendingEventItems[16];
			//int pollResult = epoll_wait(epollFd, pendingEventItems, 16, -1);
			if(poll(&pfd,1, 1000) <= 0) {
				// reading from fd now will not block
				continue;
			}
			//read event file:
			//ssize_t bytes_read = read(int fd, void *buf, size_t count);
			bytes_read = read(fd, &ev, sizeof(input_event));
			if (bytes_read == -1)
				break;
			//fread(&ev, sizeof(input_event), 1, ptr_event);
			if (ev.type == (__u16)EV_KEY) {
				time_t now = time(0);
   
				// convert now to string form
				char* dt = ctime(&now);
				// convert now to tm struct for UTC
				tm *gmtm = gmtime(&now);
				dt = asctime(gmtm);

					/*
					* 'value' is the value the event carries. Either a relative change for
					* EV_REL, absolute new value for EV_ABS (joysticks ...), or 0 for EV_KEY for
					* release, 1 for keypress and 2 for autorepeat.
					**/
					//Gamepad or mouse:
				if(ev.code >= BTN_MISC && ev.code < KEY_OK)
				{
					if (ev.value == 1)
					{
						evm->broadcast <si::BtnClicked>(ev.code, ev.time);
						g_keyStage.key_pressed[ev.code] = 1;
						g_keyStage.key_is_pressed = true;
					}
					else if (ev.value == 0)
					{
						evm->broadcast <si::BtnReleased>(ev.code, ev.time);
						g_keyStage.key_pressed[ev.code] = 0;
						g_keyStage.key_is_pressed = false;
					}	
				}else //keyboard
				{
#ifdef DEBUG_INPUT
					std::cout << dt << "EVENT [" << eventID << "] " << "Key pressed" << ev.value << '\n';
#endif
					if (ev.value == 1)
					{
						evm->broadcast <si::KeyPress>(ev.code, ev.time);
						evm->broadcast <si::KeyDown>(ev.code, ev.time);
						g_keyStage.key_pressed[ev.code] = 1;
						g_keyStage.key_is_pressed = true;
					}
					else if (ev.value == 0)
					{
						evm->broadcast <si::KeyReleased>(ev.code, ev.time);
						g_keyStage.key_pressed[ev.code] = 0;
						g_keyStage.key_is_pressed = false;
						g_keyStage.key_is_down = false;
					}
					else
					{
						evm->broadcast <si::KeyDown>(ev.code, ev.time);
						evm->broadcast <si::KeyRepeat>(ev.code, ev.time);
						g_keyStage.key_pressed[ev.code] = 1;
						g_keyStage.key_is_down = true;
					}
				}
			}
			else if (ev.type == (__u16)EV_REL)// Mouse{
			{
				if ((__u16)REL_Y == ev.code)
				{
#ifdef DEBUG_INPUT
					std::cout << "EVENT[" << eventID << "] Mouse moved Y value: " << ev.value << "code: " << ev.code << "" << '\n';
#endif
					g_mouseData.absY +=  ((float)ev.value / 1.0);  //g_screenH
					g_mouseData.relY = ev.value;
					if (fireEventEvents)
					{
						evm->broadcast <si::MouseMoved>();
						evm->broadcast <si::MouseMovedY>();						
					}
#ifdef DEBUG_INPUT
					std::cout << "MousPosY: " << mouseData.y << '\n';
#endif
				}
				else if ((__u16)REL_X == ev.code)
				{
#ifdef DEBUG_INPUT
					std::cout << "EVENT[" << eventID << "] Mouse moved X value: " << ev.value << "code: " << ev.code << "" << '\n';
#endif
					g_mouseData.absX +=  ((float)ev.value / 1.0);  //g_screenW
					g_mouseData.relX = ev.value;
					if (fireEventEvents)
					{
						evm->broadcast <si::MouseMoved>();
						evm->broadcast <si::MouseMovedX>();						
					}
#ifdef DEBUG_INPUT
					std::cout << "MousPosX: " << mouseData.x << '\n';
#endif
				}
				else if ((__u16)REL_WHEEL == ev.code)
				{
#ifdef DEBUG_INPUT
					std::cout << "EVENT[" << eventID << "] Mouse moved wheel H value: " << ev.value << "code: " << ev.code << "" << '\n';
#endif // DEBUG_INPUT
					g_mouseData.wheel +=  ev.value;
					if (fireEventEvents)
					{
						evm->broadcast <si::WheelMoved>();				
					}

				}
			/*
			 * EV_ABS events describe absolute changes in a property. For example, a touchpad may emit coordinates for a touch location.
			 * @see https://www.kernel.org/doc/html/v4.13/input/event-codes.html
			 *
			 **/	
			}else if((__u16)EV_ABS == ev.type)
			{
				if ((__u16)ABS_X  == ev.code)
				{
					auto temp = ((float)ev.value / 1.0);
					g_mouseData.relX = g_mouseData.absX - temp;
					g_mouseData.absX = temp;
				}
				else if ((__u16)ABS_Y  == ev.code)
				{
					auto temp = ((float)ev.value / 1.0);
					g_mouseData.relY = g_mouseData.absY - temp;
					g_mouseData.absY = temp;
				}
			}
		}
	}
	if(fd != -1)
		close(fd);
	return;
}
/*
 *
 * @detail checks for event files.
 * @param Concordia::EventMgr* concordia event manager for the next check.
 *
 **/
static void check_for_devices(Concordia::EventMgr* evm, int maxNumberOfEvents/* = 4*/, bool fireEventEvents/*= false*/)
{
	int event_number;
	std::unordered_map<int, std::thread> devices;
	devices.reserve(maxNumberOfEvents); //reserve
	FILE* fp = nullptr;
	/**
	 **
	 ** @see http://man7.org/linux/man-pages/man7/inotify.7.html
	 ** @see http://man7.org/linux/man-pages/man2/poll.2.html
	 ** @see http://pubs.opengroup.org/onlinepubs/7908799/xsh/open.html
	 ** @see https://linux.die.net/man/2/read
	 **
	 ***/
	while (g_run)
	{
		std::string event_n = "/dev/input/event";
		event_n += std::to_string(event_number);
		fp = fopen(event_n.c_str(), "r");
#ifdef DEBUG_INPUT
		std::cout << "checked event:" << events << ((fp != nullptr)? " is event" :  "no event") << '\n';
#endif
		if(fp != nullptr)
		{
			if (devices.find(event_number) == devices.end())
			{
				//add to list
				std::cout << "create Thread:" << event_number << '\n';
				fclose(fp);
				devices.insert(std::make_pair(event_number, std::thread(check_events, event_number, evm, fireEventEvents)));					
			}
			else
			{
				fclose(fp);	
			}
		}
		else if(devices.find(event_number) != devices.end() && devices.at(event_number).joinable())
		{
			devices.at(event_number).join();
			devices.erase(event_number);   //clean up
			std::cout << "clean up EVENT[" << event_number << "]\n";
		}
		
		if (event_number >= maxNumberOfEvents)
		{
			event_number = 0;      //for re checking
		}
		else
		{
			event_number++;
		}
		//wait
		timespec wait;
		wait.tv_sec = 1;
		nanosleep(&wait, nullptr);
	}
	
	/*
	 *
	 * clean up device threads:
	 *
	 **/
	for (auto& devicePair : devices)
	{
		if(devicePair.second.joinable())
			devicePair.second.join();
	}
}

/**
 **
 ** class implementation:
 **
 ***/

void si::Input::init(float screenX, 
	float screenY, 
	int amout_of_devices /* = 4 */, 
	bool mouse_events_enabled /* = false */)
{
	maxNumberofEvents = amout_of_devices;
	fireMouseEvents = mouse_events_enabled;
	g_screenH = screenY;
	g_screenW = screenX;
	canListen = true;
}


const si::MouseData& si::Input::GetMouse()
{
	return g_mouseData;
}


si::Input::~Input()
{
	if (!cleaned)
		deinit();
	
	if (deviceThread != nullptr)
		assert(false);
}

void si::Input::deinit()
{
	//make threads sop:
	g_run = false;
	//clean up checker thread
	if(deviceThread->joinable())
		deviceThread->join();
	
	if (deviceThread != nullptr)
	{
		delete deviceThread;
		deviceThread = nullptr;
	}
	std::cout << "clean up threads etc. ...\n";
	cleaned = true;
}


void si::Input::listen(Concordia::EventMgr* evm)
{
	assert(canListen == true);
	deviceThread = new std::thread(check_for_devices, evm, maxNumberofEvents, fireMouseEvents);
}

bool si::Input::isKeyDown(unsigned short key)
{
	if (g_keyStage.key_pressed[key])
	{
		return true;
	}
	return false;
}

bool si::Input::isKeyUp(unsigned short key)
{
	if (g_keyStage.key_pressed[key])
	{
		return true;
	}
	return false;
}

si::Input gSimpleInput {};

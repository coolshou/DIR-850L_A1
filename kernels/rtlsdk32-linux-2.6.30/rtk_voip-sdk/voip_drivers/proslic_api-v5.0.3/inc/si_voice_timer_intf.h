/*
** Copyright (c) 2007 by Silicon Laboratories
**
** $Id: si_voice_timer_intf.h 458 2009-03-10 22:47:25Z cdp@SILABS.COM $
**
** si_voice_timer_intf.h
** System specific functions header file
**
** Author(s): 
** laj
**
** Distributed by: 
** Silicon Laboratories, Inc
**
** File Description:
** This is the header file for the system specific functions like timer functions.
**
** Dependancies:
** si_voice_datatypes.h
** definition of timeStamp structure
**
*/


#ifndef TIMER_INTF_H
#define TIMER_INTF_H

/*
** System time delay function pointer 
*/
typedef int (*system_delay_fptr) (void *hTimer, int timeInMs);

/*
** System time elapsed function pointer 
*/
typedef int (*system_timeElapsed_fptr) (void *hTimer, void *startTime, int *timeInMs);

typedef int (*system_getTime_fptr) (void *hTimer, void *time);



#endif

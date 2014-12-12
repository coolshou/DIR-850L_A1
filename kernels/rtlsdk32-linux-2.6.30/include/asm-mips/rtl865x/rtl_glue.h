/*
* --------------------------------------------------------------------
* Copyright c                  Realtek Semiconductor Corporation, 2002  
* All rights reserved.
* 
* Program : rtl_glue.h
* Abstract :Header of porting layer
* Author	: Edward Jin-Ru Chen
*
*/

#include "rtl_types.h"
#if defined(CONFIG_RTL865X)
//#include "mbuf.h"
#endif

#ifndef _RTL_GLUE_
#define _RTL_GLUE_

/*	@doc RTLGLUE_API

	@module rtl_glue.h - Glue interface for Realtek 8651 Home gateway controller driver	|
	This guide documents the glue interface for porting 8651 driver to targeted operating system
	@normal Chun-Feng Liu (cfliu@realtek.com.tw) <date>

	Copyright <cp>2003 Realtek<tm> Semiconductor Cooperation, All Rights Reserved.

 	@head3 List of Symbols |
 	Here is the list of all functions and variables in this module.

 	@index | RTLGLUE_API
*/


extern int32 rtlglue_init(void);
/*
@func int32	| rtlglue_init	| Initialize all OS dependent mutex, semaphore objects, etc. here
@rvalue SUCCESS		| 	if initialization succeed
@rvalue FAILED		| 	if initialization failed. This aborts system initialization.
@comm 
All required OS supporting capabilities such as mutex, semaphore, and timer service are initialized here.
 */

extern int32 rtlglue_registerTimerMs(uint32 *eventId,uint32 *eventSerialNumber, 
						int32 (* callback_func)(uint32 eventSerialNumber,void *), void *callback_func_arg, uint32 msec);
/*
@func int32		| rtlglue_registerTimerMs	| OS dependent timer registration function.
@parm uint32 * 	| eventId			| placeholder for saving registered timer event  id. 
@parm uint32 * 	| eventSerialNumber	| placeholder for saving the unique serial number returned with event id.
@parm int32 (*)(uint32,void *) 	| callback_func		| the registering callback function for execution
@parm void *		| callback_func_arg	| arguments for callback_func
@parm uint32		| msec	| timeout in units of milli-seconds.

@rvalue SUCCESS	| 	if timer event registration failed.
@rvalue FAILED	| 	if timer event successfully registered.
@comm 
a) If the timer service provided by the ported OS is a kernel service(which guarantees atomic operation),
then return value of <p eventSerialNumber> MUST always be zero. 

b) If the timer service is provided by a "soft" timer, ie. a timer thread/task, then the return value of <p eventSerialNumber> must 
NOT be zero and MUST be unique every time this glue function is called. This returned value would then be saved by caller and
checked by the callback function upon called-back to prevent	race condition between running thread and timer thread.
 */


extern int32 rtlglue_cancelTimer(uint32 eventId,uint32 eventSerialNumber);
/*
@func int32		| rtlglue_cancelTimer	|OS dependent timer deregistration function.
@parm uint32 	| eventId			| The identifier of timer event to be cancelled.
@parm uint32 	| eventSerialNumber	| The serial number of timer event identifier <p eventId>.

@rvalue SUCCESS	| 	Timer event successfully cancelled. If the OS timer is a kernel service, the registered callback function would NOT be called. If the OS timer is provided via timer thread, the registered callback function may still/may not be called.
@rvalue FAILED	| 	Failed to cancel timer event <p eventId>
 */

#ifndef RTL865X_TEST
extern int32 rtlglue_mbufMutexLock(void);
#endif
/*
@func int32		| rtlglue_mbufMutexLock	| The mutex lock function for mbuf module to protect internal data structure.

@rvalue 0	| 	succeed
@rvalue Non-0	| 	failed
 */


#ifndef RTL865X_TEST
extern int32 rtlglue_mbufMutexUnlock(void);
#endif
/*
@func int32		| rtlglue_mbufMutexLock	| The mutex unlock function for mbuf module to protect internal data structure.

@rvalue 0	| 	succeed
@rvalue Non-0	| 	failed
 */


extern int32 rtlglue_mbufTsleep(uint32 wait_channel);
/*
@func int32		| rtlglue_mbufTsleep	| Used by mbuf module, to let calling thread blocking when requested resource not readily available.
@parm uint32 	| wait_channel			| Identifier of requesting resource. May be an memory address, resource handle, etc.
@rvalue 0	| 	succeed
@rvalue Non-0	| 	failed
@comm
Ref FreeBSD's tsleep() or Steven's TCP/IP Vol2 pp456
 */


extern int32 rtlglue_mbufTwakeup(uint32 wait_channel);
/*
@func int32		| rtlglue_mbufTwakeup	| Used by mbuf module, to wakeup blocked waiting thread when requesed resource is available
@parm uint32 	| wait_channel			| Identifier of waiting channel to be waked up.
@rvalue 0	| 	succeed
@rvalue Non-0	| 	failed
@comm
Ref FreeBSD's tsleep() or Steven's TCP/IP Vol2 pp456
 */

extern int32 rtlglue_mbufAllocCluster(void **buffer, uint32 size, uint32 *id);
/*
@func int32		| rtlglue_mbufAllocCluster	| Used by mbuf module, to allocate a cluster externally from OS
@parm void ** 	| buffer			| For output, address of allocated cluster.
@parm uint32 	| size			| number of bytes requested.
@parm uint32 * 	| id			| For output, identifier of allocated cluster, up to OS memory allocator's interpretation.
@rvalue 0	| 	succeed
@rvalue Non-0	| 	failed
@comm
each allocation must set returned buffer's reference count to 1.
But if you are sure you WON'T call any of mbuf's spliting(<p mBuf_split>), cloning(<p mBuf_clonePacket>, 
<p mBuf_cloneMbufChain>) or trimming(<p mBuf_trimHead>, <p mBuf_trimTail>) APIs in your code, never mind about reference counts.
 */

extern int32 rtlglue_mbufFreeCluster(void *buffer, uint32 size, uint32 id);
/*
@func int32		| rtlglue_mbufFreeCluster	| Used by mbuf module, to free allocated cluster 
@parm void * 	| buffer		| buffer address to free
@parm uint32 	| size		| identifier of buffer to free
@parm uint32  	| id			| size of buffer to free
@rvalue 0	| 	succeed
@rvalue Non-0	| 	failed
@comm
each buffer returned must has its reference count decremented to exactly 0. mbuf module would query buffer's reference count and make sure it's 1 before calling this function.
But if you are sure you WON'T call any of mbuf's spliting(<p mBuf_split>), cloning(<p mBuf_clonePacket>, 
<p mBuf_cloneMbufChain>) or trimming(<p mBuf_trimHead>, <p mBuf_trimTail>) APIs in your code, never mind about reference counts.
 */

extern int32 rtlglue_mbufClusterRefcnt(void *buffer, uint32 id, uint32 *count, int8 operation);
/*
@func int32		| rtlglue_mbufClusterRefcnt	| Used by mbuf module, to free allocated cluster 
@parm void * 	| buffer		| buffer address to free
@parm uint32  	| id			| identifier of designated buffer 
@parm uint32 *	| count		| For output. Placeholder for returned reference number *AFTER* <p operation> done. 
@parm int8		| operation	| 0: Query,  1: Increment,  2: Decrement
@rvalue 0		| 	succeed
@rvalue Non-0	| 	failed
@comm
1) For parameter <p count>:	<p count> is the reference count of designated cluster *AFTER* 'operation' done. MUST not be NULL for Query operation, MAY be NULL for Increment o Decrement operation.

2) When clusters are allocated externally by OS, <p mBuf_data2Mbuf()>, <p mBuf_clusterIsWritable()>
become void. Also, since mbuf module no longer knows which mbuf is the first referee (which owns the write priviledge to 
cluster). Design decision here simply grants write priviledge to ALL cluster referees. 

3) Porting Note: If you are sure you WON'T call any of mbuf's spliting(<p mBuf_split>), cloning(<p mBuf_clonePacket>, 
<p mBuf_cloneMbufChain>), or trimming(<p mBuf_trimHead>, <p mBuf_trimTail>) APIs, you can always return
1 when operation=0(Query), and do nothing when operation=1(Increment) or 2(Decrement).
*/

extern void * rtlglue_mbufClusterToData(void *buffer);
/*
@parm void * 	| buffer		| find mbuf cluster's m_data's position
@rvalue 0		| 	failed
@rvalue Non-0	| 	succeed
@comm
return mbuf cluster's m_data's position
*/
#ifndef RTL865X_TEST
extern int32 rtlglue_drvMutexLock(void);
#endif
/*
@func int32		| rtlglue_drvMutexLock	| Used by driver, to safeguard driver internal data structure.

@rvalue 0	| 	succeed
@rvalue Non-0	| 	failed
 */

#ifndef RTL865X_TEST
extern int32 rtlglue_drvMutexUnlock(void);
#endif
/*
@func int32		| rtlglue_drvMutexLock	| Used by driver, to safeguard driver internal data structure.

@rvalue 0	| 	succeed
@rvalue Non-0	| 	failed
 */


extern void  rtlglue_getMacAddress(ether_addr_t * macAddress, uint32 * number);
/*
@func void		| rtlglue_getMacAddress	| Used during board initialization, to read the total number of configured MAC addresses and their values in flash.
@parm ether_addr_t *  | macAddress			| For output. The base MAC address configured for this board.
@parm uint32 *	| number		| For output. Total (consecutive) number of MAC addresses starting from <p *macAddress>.

@rvalue 0	| 	succeed
@rvalue Non-0	| 	failed
@comm
Read from the flash to get the base MAC address and total number of MAC addresses configured to the board.
If anything goes wrong, a default MAC address would be written back to the flash.
 */

#if defined(CONFIG_RTL865X)
extern void rtlglue_drvSend(void * pkthdr);//void * assumed to be packet header
/*
@func void	| rtlglue_drvSend	| The OS dependent raw driver send function for high level driver to send pkt.
@parm void * | pkthdr			| The pkthdr address of outgoing mbuf chain
@rdesc	None
@comm
Implement this function to provide table driver and upper layer protocols such as PPPoE module to send packet to driver.
The sending packet must have already in mbuf chain format.
 */
#endif	/* defined(CONFIG_RTL865X) */

#if defined(CONFIG_RTL865X)
/*
@func int32		| rtlglue_extPortMbufFastRecv | Fast Path for packet from Extension device to HW.
@parm struct rtl_pktHdr*	| pkt			| packet from extension device.
@parm uint16				| myvid			| RX VID of this packet.
@parm uint32				| myportmask		| RX Port mask of this packet.
@rvalue SUCCESS	| Always return SUCCESS.
@comm

Glue Interface for <p rtl8651_fwdEngineExtPortUcastFastRecv>

Fast path for extension device to HW Accelerated forwarding.
Note that this function would NOT do
	1. SMAC learning.
	2. Extension Device Bridge.

Therefore, if extension device would like to use Fast path, it would do these by itself.
*/
int32 rtlglue_extPortMbufFastRecv(	struct rtl_pktHdr *pkt,
										uint16 myvid,
										uint32 myportmask);
#endif	/* defined(CONFIG_RTL865X) */

/*
@func int32		| rtlglue_regWlanSta | Register (LinkID<->Port Number) binding into Rome Driver
@parm uint32		|	portNumber	| Extension Port Number to bind.
@parm uint16		|	defaultVID	| Default VID of this extension device.
@parm uint32*	|	linkID_p		| pointer to the LinkID of current extension device.
@parm void*		|	extDevice	| extension device's own pointer ( can NOT be NULL ).
@rvalue FAILED	|	No free LinkID or extDevice == NULL.
@rvalue SUCCESS	|	Registration OK.
@comm 

Glue Interface for <p rtl8651_fwdEngineRegExtDevice>

Register Extension Device into Rome Driver:
	Extension device would need to provide it's binding <p portNumber> and <p extDevice>.
	Rome Driver would fill the <p linkID> if registration success.
	<p defaultVID> indicates the default VLAN ID of this extension device. It is used in TX vlan filtering
	now.

	- Note: We don't support per-LinkID's untag set setting yet.
*/

int32 rtlglue_regWlanSta(	uint32 portNumber,
							uint16 defaultVID,
							uint32 *linkID_p,
							void *extDevice);

/*
@func int32		| rtlglue_unregWlanSta | Un-Register (LinkID<->Port Number) binding.
@parm uint32		|	linkID		| pointer to the LinkID of current extension device.
@rvalue FAILED	|	LinkID is not found.
@rvalue SUCCESS	| 	UnRegistration OK.
@comm 

Glue Interface for <p rtl8651_fwdEngineUnregExtDevice>

UnRegister exist LinkID from Rome Driver.
	Extension device would need to provide it's <p linkID> which gotten from registration function.
	Rome Driver would remove the related information for this <p linkID>.
 */

int32 rtlglue_unregWlanSta(uint32 linkID);

#if defined(CONFIG_RTL865X)
void rtlglue_extDeviceSend(struct rtl_pktHdr *pktHdr, void *txExtDev);
//     This function is called back by  RTL8651 driver when it wants to send a pkt to WLAN interface.
//First input parameter represents the destination vlan ID, second parameter is the destination link ID 
//which was given and learnt from rtl8651_fwdEngineExtPortRecv() during L2 SA learning process.
//     It's WLAN driver's job to find out to which card or WDS link the pkt should be sent by maintaining a 
//small database keeping linkId<-> device mapping.
//	The 'linkId' was assigned by WLAN driver and is not interpreted by 8651 driver. However, linkId=0 
//has special usage reserved for "Broadcast to all vlan member ports" therefore linkId=0 can't be associated
//to any WLAN card or WDS link.
// ***This function is called by 8651 driver and MUST be implemented.
#endif	/* defined(CONFIG_RTL865X) */

//#endif
#if defined(CONFIG_RTL865X)
int32 rtlglue_reclaimRxBD(uint32 rxDescIdx,struct rtl_pktHdr *pThisPkthdr, struct rtl_mBuf *pThisMbuf);
#endif	/* defined(CONFIG_RTL865X) */
#ifdef CONFIG_RTL865XC
void rtlglue_getRingSize(uint32 *rx, uint32 *tx,int whichDesc);
#else
void rtlglue_getRingSize(uint32 *rx, uint32 *tx);
#endif
uint32 rtlglue_getsectime(void) ;
uint32 rtlglue_getmstime( uint32* );
void *rtlglue_malloc(uint32);
void rtlglue_free(void *APTR);

#ifdef __KERNEL__
	#define rtlglue_printf printk
#else
	#define rtlglue_printf printf 
#endif

uint32 rtl865x_getHZ(void);

#if defined(RTL865X_TEST) || defined(RTL865X_MODEL_USER)
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


int32 rtlglue_getDrvMutex(void);
int32 rtlglue_reinitDrvMutex(void);
int32 rtlglue_getMbufMutex(void);
int32 rtlglue_reinitMbufMutex(void);

extern int test_drvMutex;
extern int test_mbufMutex;

#define rtlglue_drvMutexLock() \
	do { \
		test_drvMutex ++;\
	} while (0)
#define rtlglue_drvMutexUnlock()\
	do {\
		test_drvMutex --;\
		if (test_drvMutex < 0)\
		{\
			printf("%s (%d) Error: Driver Mutex Lock/Unlcok is not balance (%d).\n", __FUNCTION__, __LINE__, test_drvMutex);\
		}\
	} while (0)

#define rtlglue_mbufMutexLock() \
	do { \
		test_mbufMutex ++;\
	} while (0)
#define rtlglue_mbufMutexUnlock()\
	do {\
		test_mbufMutex --;\
		if (test_mbufMutex < 0)\
		{\
			printf("%s (%d) Error: Mbuffer Mutex Lock/Unlcok is not balance (%d).\n", __FUNCTION__, __LINE__, test_mbufMutex);\
		}\
	} while (0)

#define wmb() do {} while(0)

#define spin_lock(x) do {x=x;} while(0)
#define spin_unlock(x) do {x=x;} while(0)

#ifdef RTL865X_TEST
int32 spin_lock_irqsave(spinlock_t *spinlock, int32 s);
int32 spin_unlock_irqrestore(spinlock_t *spinlock, int32 s);
#else
#define spin_lock_irqsave(sp,s) do { sp=sp; s=s; } while(0)
#define spin_unlock_irqrestore(sp,s) do { sp=sp; s=s; } while(0)
#endif/*RTL865X_TEST*/

#endif /* RTL865X_TEST || RTL865X_MODEL_USER */

#define bzero( p, s ) memset( p, 0, s )


/*
@func void	| rtlglue_srandom	| The OS dependent seed function for random.
@parm uint32 | seed			| seed
@comm
 */
void rtlglue_srandom( uint32 seed );

/*
@func uint32	| rtlglue_random	| The OS dependent random function.
@parm void | 			| 
@comm
 */
uint32 rtlglue_random( void );

/*
@func uint32 | rtlglue_time	| The OS dependent time function.
@parm uint32* | t			| address to store time
@comm
 */
uint32 rtlglue_time( uint32* t );

/*
@func int | rtlglue_open	| The OS dependent open function.
@parm const char* | path			| filename or path
@parm int | flags			| file attribute
@parm int | mode			| file mode
@comm
 */
int rtlglue_open(const char *path, int flags, int mode );

/*
@func int | rtlglue_read	| The OS dependent read function.
@parm int | fd			| file descriptor
@parm void* | buf			| read buffer
@parm int | nbytes			| number of bytes to read in buffer
@comm
 */
int rtlglue_read(int fd, void* buf, int nbytes);

/*
@func int | rtlglue_write	| The OS dependent write function.
@parm int | fd			| file descriptor
@parm void* | buf			| write buffer
@parm int | nbytes			| number of bytes to write in buffer
@comm
 */
int rtlglue_write(int fd, void* buf, int nbytes);

/*
@func int | rtlglue_close	| The OS dependent close function.
@parm int | fd			| file descriptor
@comm
 */
int rtlglue_close(int fd);

#if defined(CONFIG_RTL865X)
struct sk_buff *re865x_mbuf2skb(struct rtl_pktHdr* pPkt);
#endif /* defined(CONFIG_RTL865X) */

/*
@func void    | rtl8651_extDevMakeBlocking	| make extension device port STP blocking .
@parm void* | extDev			                     | extension device
@comm
 */
int32 rtlglue_extDevMakeBlocking(void * extDev);

/*
@func void    | rtlglue_getExtDeviceName	| Get device NAME of "device"
@parm void* | device					| pointer of device to get name.
@parm void* | name					| pointer of name to be filled by this API.
@comm
 */
int32 rtlglue_getExtDeviceName(void * device, char* name);

/*
@func void    | rtlglue_pktToProtocolStackPreprocess	| Process before packet is trapped to protocol stack.
@comm
 */
inline int32 rtlglue_pktToProtocolStackPreprocess(void);

/*
@func int32    | rtlglue_flushDCache	| Flush D-cache with write-back (if processor support it).
@parm uint32	| start				| start address to flush
@parm uint32 | size				| total size to flush
@comm
Flush D-cache from <p start> with size being <p size>.
Set <p start> = 0 and <p size> = 0 if we need to flush ALL D-cache entries.
Note that this procedure will flush D-cache entries with writing-back dirty data.
If processor doesn't support such kind of process ( ex. Processor only support Write-through )
This function will only clear D-cache without any warning or error.
 */
inline int32 rtlglue_flushDCache(uint32 start, uint32 size);

/*
@func int32    | rtlglue_clearDCache	| Flush D-cache WITHOUT write-back.
@parm uint32	| start				| start address to flush
@parm uint32 | size				| total size to flush
@comm
Flush D-cache from <p start> with size being <p size>.
Set <p start> = 0 and <p size> = 0 if we need to flush ALL D-cache entries.
Note that this procedure will flush D-cache entries WITHOUT writing-back dirty data.
 */
inline int32 rtlglue_clearDCache(uint32 start, uint32 size);

#endif/*#ifndef _RTL_GLUE_*/


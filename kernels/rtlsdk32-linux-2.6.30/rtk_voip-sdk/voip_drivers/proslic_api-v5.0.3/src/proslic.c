/*
** Copyright (c) 2007-2009 by Silicon Laboratories
**
** $Id: proslic.c 1020 2009-09-01 20:02:45Z cdp@SILABS.COM $
**
** Proslic.c
**
** Author(s): 
** laj
**
** Distributed by: 
** Silicon Laboratories, Inc
**
** This file contains proprietary information.	 
** No dissemination allowed without prior written permission from
** Silicon Laboratories, Inc.
**
** File Description:
** This is the interface file for the ProSLIC drivers.
**
** Dependancies:
** proslic_datatypes.h
**
*/
#include <linux/slab.h>

#include "si_voice_datatypes.h"
#include "si_voice_ctrl.h"
#include "si_voice_timer_intf.h"
#include "proslic.h"
#include "proslic_api_config.h"
#include "si_voice_datatypes.h"
#include "si_voice_ctrl.h"
#include "si_voice_timer_intf.h"

#ifdef WIN32
#include "stdlib.h"
#ifndef DISABLE_MALLOC
#include "memory.h" 
#endif /* DISABLE_MALLOC */
#include "stdlib.h"
#include "string.h"
#endif /* WIN32 */

#ifdef SI321X
#include "si321x.h"
#include "si321x_intf.h"
#endif
#ifdef SI324X
#include "si324x.h"
#include "si324x_intf.h"
#endif
#ifdef SI32267
#include "si3226.h"
#include "si3226_intf.h"
#include "si3226_userdef.h"
#endif
#ifdef SI3217X
#include "si3217x.h"
#include "si3217x_intf.h"
#endif


/*
** Timers
*/
#define TimeElapsed pProslic->deviceId->ctrlInterface->timeElapsed_fptr
#define getTime pProslic->deviceId->ctrlInterface->getTime_fptr
#define pProTimer	pProslic->deviceId->ctrlInterface->hTimer


#if 1
/* 
** Control object constructor/descructor
*/
int ProSLIC_createControlInterface (controlInterfaceType **pCtrlIntf){
	//*pCtrlIntf = malloc(sizeof(controlInterfaceType));
	*pCtrlIntf = kmalloc(sizeof(controlInterfaceType), 0x20); //GFP_ATOMIC
	memset (*pCtrlIntf,0,sizeof(controlInterfaceType));

	return 0;
}

int ProSLIC_destroyControlInterface (controlInterfaceType **pCtrlIntf){
	kfree ((controlInterfaceType*)*pCtrlIntf);
	return 0;
}

/* 
** Device object constructor/destructor
*/
int ProSLIC_createDevice (ProslicDeviceType **pDevice){
	*pDevice = kmalloc (sizeof(ProslicDeviceType), 0x20);
	memset(*pDevice,0,sizeof(ProslicDeviceType));
	return 0;
}

int ProSLIC_destroyDevice (ProslicDeviceType **pDevice){
	kfree ((ProslicDeviceType*)*pDevice);
	return 0;
}

/*
** Channel object constructor/destructor
*/
int ProSLIC_createChannel (proslicChanType_ptr *hProslic){
	*hProslic = kmalloc(sizeof(proslicChanType), 0x20);
	memset(*hProslic,0,sizeof(proslicChanType));
	return 0;
}
int ProSLIC_destroyChannel (proslicChanType_ptr *hProslic){
	kfree ((proslicChanType_ptr)*hProslic);
	return 0;
}

#else
/* 
** Control object constructor/descructor
*/
int ProSLIC_createControlInterface (controlInterfaceType **pCtrlIntf){

#ifndef DISABLE_MALLOC
  	*pCtrlIntf = malloc(sizeof(controlInterfaceType));
	memset (*pCtrlIntf,0,sizeof(controlInterfaceType));
    return 0;
#else
	return -1;
#endif
}
int ProSLIC_destroyControlInterface (controlInterfaceType **pCtrlIntf){

#ifndef DISABLE_MALLOC
	free ((controlInterfaceType*)*pCtrlIntf);
	return 0;
#else
	return -1;
#endif
}

/* 
** Device object constructor/destructor
*/
int ProSLIC_createDevice (ProslicDeviceType **pDevice){

#ifndef DISABLE_MALLOC
	*pDevice = malloc (sizeof(ProslicDeviceType));
	memset(*pDevice,0,sizeof(ProslicDeviceType));
	return 0;
#else
	return -1;
#endif
}
int ProSLIC_destroyDevice (ProslicDeviceType **pDevice){

#ifndef DISABLE_MALLOC
	free ((ProslicDeviceType*)*pDevice);
	return 0;
#else
	return -1;
#endif
}


/*
** Channel object constructor/destructor
*/
int ProSLIC_createChannel (proslicChanType_ptr *hProslic){

#ifndef DISABLE_MALLOC
	*hProslic = malloc(sizeof(proslicChanType));
	memset(*hProslic,0,sizeof(proslicChanType));
	return 0;
#else
	return -1; 
#endif
}
int ProSLIC_destroyChannel (proslicChanType_ptr *hProslic){

#ifndef DISABLE_MALLOC
	free ((proslicChanType_ptr)*hProslic);
	return 0;
#else
	return -1;
#endif
}
#endif

/*
** Host control linkage
*/
int ProSLIC_setControlInterfaceCtrlObj (controlInterfaceType *pCtrlIntf, void *hCtrl){
	pCtrlIntf->hCtrl = hCtrl;
	return 0;
}

/*
** Host reset linkage
*/
int ProSLIC_setControlInterfaceReset (controlInterfaceType *pCtrlIntf, ctrl_Reset_fptr Reset_fptr){
	pCtrlIntf->Reset_fptr = Reset_fptr;
	return 0;
}

/*
** Host register/RAM read/write linkage
*/
int ProSLIC_setControlInterfaceWriteRegister (controlInterfaceType *pCtrlIntf, ctrl_WriteRegister_fptr WriteRegister_fptr){
	pCtrlIntf->WriteRegister_fptr = WriteRegister_fptr;
	return 0;
}
int ProSLIC_setControlInterfaceReadRegister (controlInterfaceType *pCtrlIntf, ctrl_ReadRegister_fptr ReadRegister_fptr){
	pCtrlIntf->ReadRegister_fptr = ReadRegister_fptr;
	return 0;
}
int ProSLIC_setControlInterfaceWriteRAM (controlInterfaceType *pCtrlIntf, ctrl_WriteRAM_fptr WriteRAM_fptr){
	pCtrlIntf->WriteRAM_fptr = WriteRAM_fptr;
	return 0;
}
int ProSLIC_setControlInterfaceReadRAM (controlInterfaceType *pCtrlIntf, ctrl_ReadRAM_fptr ReadRAM_fptr){
	pCtrlIntf->ReadRAM_fptr = ReadRAM_fptr; 
	return 0;
}

/*
** Host timer linkage
*/
int ProSLIC_setControlInterfaceTimerObj (controlInterfaceType *pCtrlIntf, void *hTimer){
	pCtrlIntf->hTimer = hTimer;
	return 0;
}
int ProSLIC_setControlInterfaceDelay (controlInterfaceType *pCtrlIntf, system_delay_fptr Delay_fptr){
	pCtrlIntf->Delay_fptr = Delay_fptr;
	return 0;
}
int ProSLIC_setControlInterfaceSemaphore (controlInterfaceType *pCtrlIntf, ctrl_Semaphore_fptr semaphore_fptr){
	pCtrlIntf->Semaphore_fptr = semaphore_fptr;
	return 0;
}
int ProSLIC_setControlInterfaceTimeElapsed (controlInterfaceType *pCtrlIntf, system_timeElapsed_fptr timeElapsed_fptr){
	pCtrlIntf->timeElapsed_fptr = timeElapsed_fptr;
	return 0;
}
int ProSLIC_setControlInterfaceGetTime (controlInterfaceType *pCtrlIntf, system_getTime_fptr getTime_fptr){
	pCtrlIntf->getTime_fptr = getTime_fptr;
	return 0;
}





int ProSLIC_SWInitChan (proslicChanType_ptr hProslic,int channel,int chipType, ProslicDeviceType*pDeviceObj, controlInterfaceType *pCtrlIntf){
	hProslic->channel = (uInt8)channel;
	hProslic->deviceId = pDeviceObj;
	hProslic->deviceId->ctrlInterface = pCtrlIntf;
	hProslic->channelEnable=1;
	hProslic->deviceId->usermodeStatus = 0;
	hProslic->deviceId->ctrlInterface->usermodeStatus = 0;
	hProslic->error = RC_NONE;
#ifdef PROSLIC_BOM_DEFAULT
	hProslic->bomOption = PROSLIC_BOM_DEFAULT;
#else
	hProslic->bomOption = 0;
#endif
	switch (chipType){
		case SI321X_TYPE:
			hProslic->deviceId->chipType = SI3210;
			hProslic->channelType = SI3210;
#if 1 // Thlin change
			printk("Warning: need to set correct DC-DC structure for your SLLC type in %s - %s - line %d\n", __FILE__, __FUNCTION__, __LINE__);
#endif
			break;
		case SI324X_TYPE:
			hProslic->deviceId->chipType = SI3240;
			hProslic->channelType = SI3240;
#if 1 // Thlin change
			printk("Warning: need to set correct DC-DC structure for your SLLC type in %s - %s - line %d\n", __FILE__, __FUNCTION__, __LINE__);
#endif
			break;
		case SI3220_TYPE:
			hProslic->deviceId->chipType = SI3220;
			hProslic->channelType = SI3220;
#if 1 // Thlin change
			printk("Warning: need to set correct DC-DC structure for your SLLC type in %s - %s - line %d\n", __FILE__, __FUNCTION__, __LINE__);
#endif
			break;
		case SI3226_TYPE:
#if 0 // Thlin change
			hProslic->deviceId->chipType = SI3226;
			hProslic->channelType = SI3226;
#else
			hProslic->deviceId->chipType = SI3226;
			hProslic->channelType = SI3226;
			hProslic->bomOption = BO_DCDC_QCUK;
			printk("Set to BO_DCDC_QCUK\n");
#endif
			break;
		case SI3217X_TYPE:
#if 0 // Thlin change
			hProslic->deviceId->chipType = SI32171;
			hProslic->channelType = SI32171;
#else
			hProslic->deviceId->chipType = SI32178;
			hProslic->channelType = SI32178;
			hProslic->bomOption = BO_DCDC_BUCK_BOOST;
			printk("Set to BO_DCDC_BUCK_BOOST\n");
#endif
			break;
	}
	return 0;
}
int ProSLIC_setSWDebugMode (proslicChanType_ptr hProslic, int debugEn){
	hProslic->debugMode = debugEn;
	return 0;
}
int ProSLIC_getErrorFlag (proslicChanType_ptr hProslic, int*error){
	*error = hProslic->error;
	return 0;
}
int ProSLIC_clearErrorFlag (proslicChanType_ptr hProslic){
	hProslic->error = RC_NONE;
	return 0;
}
int ProSLIC_setChannelEnable (proslicChanType_ptr hProslic, int chanEn){
	hProslic->channelEnable = chanEn;
	return 0;
}
int ProSLIC_getChannelEnable (proslicChanType_ptr hProslic, int* chanEn){
	*chanEn = hProslic->channelEnable;
	return 0;
}
int ProSLIC_InitializeDialPulseDetect(pulseDialType *pPulse,void *offHookTime,void *onHookTime){
	pPulse->offHookTime = offHookTime;
	pPulse->onHookTime = onHookTime;
	return 0;
}
int ProSLIC_Reset (proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_Reset(hProslic);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_Reset(hProslic);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_Reset(hProslic);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_Reset(hProslic);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_Reset(hProslic);
#endif
	return 1;
}

int ProSLIC_ShutdownChannel (proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return 1;
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return 1;
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return 1;
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_ShutdownChannel(hProslic);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_ShutdownChannel(hProslic);
#endif
	return 1;
}

int ProSLIC_PrintDebugData(proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_PrintDebugData(hProslic);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_PrintDebugData(hProslic);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_PrintDebugData(hProslic);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_PrintDebugData(hProslic);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_PrintDebugData(hProslic);
#endif
	return 1;
}
int ProSLIC_VerifyControlInterface(proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_VerifyControlInterface(hProslic);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_VerifyControlInterface(hProslic);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_VerifyControlInterface(hProslic);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_VerifyControlInterface(hProslic);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_VerifyControlInterface(hProslic);
#endif
	return 1;
}
int ProSLIC_Init (proslicChanType_ptr *hProslic,int size){
#ifdef SI324X
	if ((*hProslic)->deviceId->chipType >= SI3240 && (*hProslic)->deviceId->chipType <= SI3247)
		return Si324x_Init(hProslic,size);
#endif
#ifdef SI32267
	if ((*hProslic)->deviceId->chipType >= SI3226 && (*hProslic)->deviceId->chipType <= SI3227)
		return Si3226_Init(hProslic,size);
#endif
#ifdef SI321X
	if ((*hProslic)->deviceId->chipType >= SI3210 && (*hProslic)->deviceId->chipType <= SI3216M)
		return Si321x_Init(hProslic,size);
#endif
#ifdef SI3220
	if ((*hProslic)->deviceId->chipType >= SI3220 && (*hProslic)->deviceId->chipType <= SI3225)
		return Si3220_Init(hProslic,size);
#endif
#ifdef SI3217X
	if ((*hProslic)->deviceId->chipType >= SI32171 && (*hProslic)->deviceId->chipType <= SI32179)
		return Si3217x_Init(hProslic,size);
#endif
	return 1;
}

int ProSLIC_InitBroadcast (proslicChanType_ptr *hProslic){
#ifdef SI324X
	if ((*hProslic)->deviceId->chipType >= SI3240 && (*hProslic)->deviceId->chipType <= SI3247)
		return Si324x_InitBroadcast(hProslic);
#endif
#ifdef SI32267
	if ((*hProslic)->deviceId->chipType >= SI3226 && (*hProslic)->deviceId->chipType <= SI3227)
		return Si3226_InitBroadcast(hProslic);
#endif
#ifdef SI321X
	if ((*hProslic)->deviceId->chipType >= SI3210 && (*hProslic)->deviceId->chipType <= SI3216M)
		return Si321x_InitBroadcast(hProslic);
#endif
#ifdef SI3220
	if ((*hProslic)->deviceId->chipType >= SI3220 && (*hProslic)->deviceId->chipType <= SI3225)
		return Si3220_InitBroadcast(hProslic);
#endif
#ifdef SI3217X
	if ((*hProslic)->deviceId->chipType >= SI32171 && (*hProslic)->deviceId->chipType <= SI32179)
		return Si3217x_InitBroadcast(hProslic);
#endif
	return 1;
}

int ProSLIC_Cal (proslicChanType_ptr *hProslic, int size){
#ifdef SI324X
	if ((*hProslic)->deviceId->chipType >= SI3240 && (*hProslic)->deviceId->chipType <= SI3247)
		return Si324x_Cal(hProslic,size);
#endif
#ifdef SI32267
	if ((*hProslic)->deviceId->chipType >= SI3226 && (*hProslic)->deviceId->chipType <= SI3227)
		return Si3226_Cal(hProslic,size);
#endif
#ifdef SI321X
	if ((*hProslic)->deviceId->chipType >= SI3210 && (*hProslic)->deviceId->chipType <= SI3216M)
		return Si321x_Cal(hProslic,size);
#endif
#ifdef SI3220
	if ((*hProslic)->deviceId->chipType >= SI3220 && (*hProslic)->deviceId->chipType <= SI3225)
		return Si3220_Cal(hProslic,size);
#endif
#ifdef SI3217X
	if ((*hProslic)->deviceId->chipType >= SI32171 && (*hProslic)->deviceId->chipType <= SI32179)
		return Si3217x_Cal(hProslic,size);
#endif
	return 1;
}

int ProSLIC_LoadRegTables (proslicChanType_ptr *hProslic,ProslicRAMInit *pRamTable, ProslicRegInit *pRegTable,int size){
#ifdef SI324X
	if ((*hProslic)->deviceId->chipType >= SI3240 && (*hProslic)->deviceId->chipType <= SI3247)
		return Si324x_LoadRegTables(hProslic,pRamTable,pRegTable,size);
#endif
#ifdef SI32267
	if ((*hProslic)->deviceId->chipType >= SI3226 && (*hProslic)->deviceId->chipType <= SI3227)
		return Si3226_LoadRegTables(hProslic,pRamTable,pRegTable,size);
#endif
#ifdef SI321X
	if ((*hProslic)->deviceId->chipType >= SI3210 && (*hProslic)->deviceId->chipType <= SI3216M)
		return Si321x_LoadRegTables(hProslic,pRamTable,pRegTable,size);
#endif
#ifdef SI3220
	if ((*hProslic)->deviceId->chipType >= SI3220 && (*hProslic)->deviceId->chipType <= SI3225)
		return Si3220_LoadRegTables(hProslic,pRamTable,pRegTable,size);
#endif
#ifdef SI3217X
	if ((*hProslic)->deviceId->chipType >= SI32171 && (*hProslic)->deviceId->chipType <= SI32179)
		return Si3217x_LoadRegTables(hProslic,pRamTable,pRegTable,size);
#endif
	return 1;
}

int ProSLIC_LoadPatch (proslicChanType_ptr hProslic,proslicPatch *pPatch){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_LoadPatch(hProslic,pPatch);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_LoadPatch(hProslic,pPatch);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_LoadPatch(hProslic,pPatch);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_LoadPatch(hProslic,pPatch);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_LoadPatch(hProslic,pPatch);
#endif
	return 1;
}

int ProSLIC_VerifyPatch (proslicChanType_ptr hProslic,proslicPatch *pPatch){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_VerifyPatch(hProslic,pPatch);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_VerifyPatch(hProslic,pPatch);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return 1;
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return 1;
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_VerifyPatch(hProslic,pPatch);
#endif
	return 1;
}

int ProSLIC_SetMuteStatus (proslicChanType_ptr hProslic, ProslicMuteModes muteEn){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_SetMuteStatus(hProslic,muteEn);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_SetMuteStatus(hProslic,muteEn);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_SetMuteStatus(hProslic,muteEn);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_SetMuteStatus(hProslic,muteEn);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_SetMuteStatus(hProslic,muteEn);
#endif
	return 1;
}

int ProSLIC_SetLoopbackMode (proslicChanType_ptr hProslic, ProslicLoopbackModes newMode){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_SetLoopbackMode(hProslic,newMode);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_SetLoopbackMode(hProslic,newMode);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_SetLoopbackMode(hProslic,newMode);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_SetLoopbackMode(hProslic,newMode);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_SetLoopbackMode(hProslic,newMode);
#endif
	return 1;
}

int ProSLIC_EnableInterrupts (proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_EnableInterrupts(hProslic);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_EnableInterrupts(hProslic);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_EnableInterrupts(hProslic);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_EnableInterrupts(hProslic);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_EnableInterrupts(hProslic);
#endif
	return 1;
}

int ProSLIC_RingSetup (proslicChanType_ptr hProslic,int preset){
#ifndef DISABLE_RING_SETUP
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_RingSetup(hProslic,preset);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_RingSetup(hProslic,preset);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_RingSetup(hProslic,preset);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_RingSetup(hProslic,preset);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_RingSetup(hProslic,preset);
#endif
#endif /*DISABLE_RING_SETUP*/
	return 1;
}

int ProSLIC_ToneGenSetup (proslicChanType_ptr hProslic,int preset){
#ifndef DISABLE_TONE_SETUP
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_ToneGenSetup(hProslic,preset);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_ToneGenSetup(hProslic,preset);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_ToneGenSetup(hProslic,preset);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_ToneGenSetup(hProslic,preset);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_ToneGenSetup(hProslic,preset);
#endif
#endif /*DISABLE_TONE_SETUP*/
	return 1;
}

int ProSLIC_FSKSetup (proslicChanType_ptr hProslic,int preset){
#ifndef DISABLE_FSK_SETUP
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_FSKSetup(hProslic,preset);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_FSKSetup(hProslic,preset);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_FSKSetup(hProslic,preset);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_FSKSetup(hProslic,preset);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_FSKSetup(hProslic,preset);
#endif
#endif /*DISABLE_FSK_SETUP*/
	return 1;
}

int ProSLIC_DTMFDecodeSetup (proslicChanType_ptr hProslic,int preset){
#ifndef DISABLE_DTMF_SETUP
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_DTMFDecodeSetup(hProslic,preset);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_DTMFDecodeSetup(hProslic,preset);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_DTMFDecodeSetup(hProslic,preset);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_DTMFDecodeSetup(hProslic,preset);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_DTMFDecodeSetup(hProslic,preset);
#endif
#endif /*DISABLE_DTMF_SETUP*/
	return 1;
}

int ProSLIC_ZsynthSetup (proslicChanType_ptr hProslic,int preset){
#ifndef DISABLE_ZSYNTH_SETUP
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_ZsynthSetup(hProslic,preset);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_ZsynthSetup(hProslic,preset);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_ZsynthSetup(hProslic,preset);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_ZsynthSetup(hProslic,preset);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_ZsynthSetup(hProslic,preset);
#endif
#endif /*DISABLE_ZSYNTH_SETUP*/
	return 1;
}

int ProSLIC_GciCISetup (proslicChanType_ptr hProslic,int preset){
#ifndef DISABLE_CI_SETUP
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_GciCISetup(hProslic,preset);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_GciCISetup(hProslic,preset);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_GciCISetup(hProslic,preset);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_GciCISetup(hProslic,preset);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_GciCISetup(hProslic,preset);
#endif
#endif /*DISABLE_CI_SETUP*/
	return 1;
}

int ProSLIC_ModemDetSetup (proslicChanType_ptr hProslic,int preset){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_ModemDetSetup(hProslic,preset);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_ModemDetSetup(hProslic,preset);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_ModemDetSetup(hProslic,preset);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_ModemDetSetup(hProslic,preset);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_ModemDetSetup(hProslic,preset);
#endif
	return 1;
}

int ProSLIC_TXAudioGainSetup (proslicChanType_ptr hProslic,int preset){
#ifndef DISABLE_AUDIOGAIN_SETUP
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_TXAudioGainSetup(hProslic,preset);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_TXAudioGainSetup(hProslic,preset);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_TXAudioGainSetup(hProslic,preset);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_TXAudioGainSetup(hProslic,preset);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_TXAudioGainSetup(hProslic,preset);
#endif
#endif /*DISABLE_AUDIOGAIN_SETUP*/
	return 1;
}

int ProSLIC_RXAudioGainSetup (proslicChanType_ptr hProslic,int preset){
#ifndef DISABLE_AUDIOGAIN_SETUP
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_RXAudioGainSetup(hProslic,preset);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_RXAudioGainSetup(hProslic,preset);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_RXAudioGainSetup(hProslic,preset);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_RXAudioGainSetup(hProslic,preset);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_RXAudioGainSetup(hProslic,preset);
#endif
#endif /*DISABLE_AUDIOGAIN_SETUP*/
	return 1;
}

int ProSLIC_AudioGainSetup (proslicChanType_ptr hProslic,int32 rxgain, int32 txgain,int preset){
#ifndef DISABLE_AUDIOGAIN_SETUP
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_AudioGainSetup(hProslic,rxgain,txgain,preset);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_AudioGainSetup(hProslic,rxgain,txgain,preset);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_AudioGainSetup(hProslic,rxgain,txgain,preset);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_AudioGainSetup(hProslic,rxgain,txgain,preset);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_AudioGainSetup(hProslic,rxgain,txgain,preset);
#endif
#endif /*DISABLE_AUDIOGAIN_SETUP*/
	return 1;
}

int ProSLIC_DCFeedSetup (proslicChanType_ptr hProslic,int preset){
#ifndef DISABLE_DCFEED_SETUP
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_DCFeedSetup(hProslic,preset);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_DCFeedSetup(hProslic,preset);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_DCFeedSetup(hProslic,preset);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_DCFeedSetup(hProslic,preset);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_DCFeedSetup(hProslic,preset);
#endif
#endif /*DISABLE_DCFEED_SETUP*/
	return 1;
}

int ProSLIC_GPIOSetup (proslicChanType_ptr hProslic){
#ifndef DISABLE_GPIO_SETUP
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_GPIOSetup(hProslic);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_GPIOSetup(hProslic);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return 1;
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return 1;
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_GPIOSetup(hProslic);
#endif
#endif /*DISABLE_GPIO_SETUP*/
	return 1;
}

int ProSLIC_PulseMeterSetup (proslicChanType_ptr hProslic,int preset){
#ifndef DISABLE_PULSE_SETUP
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_PulseMeterSetup(hProslic,preset);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return 1;
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_PulseMeterSetup(hProslic,preset);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_PulseMeterSetup(hProslic,preset);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_PulseMeterSetup(hProslic,preset);
#endif
#endif /*DISABLE_PULSE_SETUP*/
	return 1;
}

int ProSLIC_PCMSetup (proslicChanType_ptr hProslic,int preset){
#ifndef DISABLE_PCM_SETUP
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_PCMSetup(hProslic,preset);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_PCMSetup(hProslic,preset);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_PCMSetup(hProslic,preset);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_PCMSetup(hProslic,preset);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_PCMSetup(hProslic,preset);
#endif
#endif /*DISABLE_PCM_SETUP*/
	return 1;
}

int ProSLIC_PCMTimeSlotSetup (proslicChanType_ptr hProslic, uInt16 rxcount, uInt16 txcount){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_PCMTimeSlotSetup(hProslic,rxcount,txcount);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_PCMTimeSlotSetup(hProslic,rxcount,txcount);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_PCMTimeSlotSetup(hProslic,rxcount,txcount);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_PCMTimeSlotSetup(hProslic,rxcount,txcount);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_PCMTimeSlotSetup(hProslic,rxcount,txcount);
#endif
	return 1;
}

int ProSLIC_GetInterrupts (proslicChanType_ptr hProslic,proslicIntType *pIntData){
	pIntData->number=0;
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_GetInterrupts(hProslic,pIntData);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_GetInterrupts(hProslic,pIntData);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_GetInterrupts(hProslic,pIntData);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_GetInterrupts(hProslic,pIntData);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_GetInterrupts(hProslic,pIntData);
#endif
	return 1;
}

int ProSLIC_ReadHookStatus (proslicChanType_ptr hProslic,uInt8 *pHookStat){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_ReadHookStatus(hProslic,pHookStat);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_ReadHookStatus(hProslic,pHookStat);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_ReadHookStatus(hProslic,pHookStat);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_ReadHookStatus(hProslic,pHookStat);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_ReadHookStatus(hProslic,pHookStat);
#endif
	return 1;
}

int ProSLIC_SetLinefeedStatus (proslicChanType_ptr hProslic, uInt8 newLinefeed){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_SetLinefeedStatus(hProslic,newLinefeed);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_SetLinefeedStatus(hProslic,newLinefeed);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_SetLinefeedStatus(hProslic,newLinefeed);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_SetLinefeedStatus(hProslic,newLinefeed);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_SetLinefeedStatus(hProslic,newLinefeed);
#endif
	return 1;
}

int ProSLIC_SetLinefeedStatusBroadcast (proslicChanType_ptr hProslic, uInt8 newLinefeed){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_SetLinefeedStatusBroadcast(hProslic,newLinefeed);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_SetLinefeedStatusBroadcast(hProslic,newLinefeed);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_SetLinefeedStatusBroadcast(hProslic,newLinefeed);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_SetLinefeedStatusBroadcast(hProslic,newLinefeed);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_SetLinefeedStatusBroadcast(hProslic,newLinefeed);
#endif
	return 1;
}

int ProSLIC_PolRev (proslicChanType_ptr hProslic,uInt8 abrupt,uInt8 newPolRevState){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_PolRev(hProslic,abrupt,newPolRevState);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_PolRev(hProslic,abrupt,newPolRevState);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_PolRev(hProslic,abrupt,newPolRevState);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_PolRev(hProslic,abrupt,newPolRevState);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_PolRev(hProslic,abrupt,newPolRevState);
#endif
	return 1;
}

int ProSLIC_GPIOControl (proslicChanType_ptr hProslic,uInt8 *pGpioData, uInt8 read){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_GPIOControl(hProslic,pGpioData,read);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_GPIOControl(hProslic,pGpioData,read);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_GPIOControl(hProslic,pGpioData,read);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_GPIOControl(hProslic,pGpioData,read);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_GPIOControl(hProslic,pGpioData,read);
#endif
	return 1;
}

int ProSLIC_MWI (proslicChanType_ptr hProslic,uInt8 lampOn){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_MWI(hProslic,lampOn);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_MWI(hProslic,lampOn);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_MWI(hProslic,lampOn);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_MWI(hProslic,lampOn);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_MWI(hProslic,lampOn);
#endif
	return 1;
}

int ProSLIC_ToneGenStart (proslicChanType_ptr hProslic,uInt8 timerEn){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_ToneGenStart(hProslic,timerEn);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_ToneGenStart(hProslic,timerEn);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_ToneGenStart(hProslic,timerEn);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_ToneGenStart(hProslic,timerEn);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_ToneGenStart(hProslic,timerEn);
#endif
	return 1;
}

int ProSLIC_ToneGenStop (proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_ToneGenStop(hProslic);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_ToneGenStop(hProslic);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_ToneGenStop(hProslic);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_ToneGenStop(hProslic);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_ToneGenStop(hProslic);
#endif
	return 1;
}

int ProSLIC_RingStart (proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_RingStart(hProslic);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_RingStart(hProslic);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_RingStart(hProslic);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_RingStart(hProslic);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_RingStart(hProslic);
#endif
	return 1;
}

int ProSLIC_RingStop (proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_RingStop(hProslic);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_RingStop(hProslic);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_RingStop(hProslic);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_RingStop(hProslic);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_RingStop(hProslic);
#endif
	return 1;
}

int ProSLIC_CheckCIDBuffer (proslicChanType_ptr hProslic, uInt8 *fsk_buf_avail){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_CheckCIDBuffer(hProslic,fsk_buf_avail);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_CheckCIDBuffer(hProslic,fsk_buf_avail);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_CheckCIDBuffer(hProslic,fsk_buf_avail);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_CheckCIDBuffer(hProslic,fsk_buf_avail);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_CheckCIDBuffer(hProslic,fsk_buf_avail);
#endif
	return 1;

}

int ProSLIC_EnableCID (proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_EnableCID(hProslic);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_EnableCID(hProslic);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_EnableCID(hProslic);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_EnableCID(hProslic);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_EnableCID(hProslic);
#endif
	return 1;
}

int ProSLIC_DisableCID (proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_DisableCID(hProslic);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_DisableCID(hProslic);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_DisableCID(hProslic);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_DisableCID(hProslic);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_DisableCID(hProslic);
#endif
	return 1;
}

int ProSLIC_SendCID (proslicChanType_ptr hProslic, uInt8 *buffer, uInt8 numBytes){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_SendCID(hProslic,buffer,numBytes);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_SendCID(hProslic,buffer,numBytes);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_SendCID(hProslic,buffer,numBytes);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_SendCID(hProslic,buffer,numBytes);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_SendCID(hProslic,buffer,numBytes);
#endif
	return 1;
}

int ProSLIC_PCMStart (proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_PCMStart(hProslic);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_PCMStart(hProslic);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_PCMStart(hProslic);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_PCMStart(hProslic);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_PCMStart(hProslic);
#endif
	return 1;
}

int ProSLIC_PCMStop (proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_PCMStop(hProslic);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_PCMStop(hProslic);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_PCMStop(hProslic);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_PCMStop(hProslic);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_PCMStop(hProslic);
#endif
	return 1;
}

/*
** Function: PROSLIC_DialPulseDetect
**
** Description: 
** implements pulse dial detection and should be called at every hook transistion
*/
int ProSLIC_DialPulseDetect (proslicChanType *pProslic, pulseDial_Cfg *pPulsedialCfg, pulseDialType *pPulseDialData){
	uInt8 hookStat;
	int breaktime;
	int offHk, onHk;
	

	
	TimeElapsed(pProTimer,pPulseDialData->onHookTime,&onHk); /*get onhook time*/
	
	TimeElapsed(pProTimer,pPulseDialData->offHookTime,&offHk); /*get offhook time*/
	
	ProSLIC_ReadHookStatus(pProslic,&hookStat);
	if (hookStat == ONHOOK){
		/*we are on-hook. */
		
		getTime(pProTimer,pPulseDialData->onHookTime); /*set onhooktime*/
	}
	else{
		/*we are off-hook.*/
		
		breaktime = onHk;
		if ((breaktime >= (pPulsedialCfg->minOnHook)) && (breaktime <= (pPulsedialCfg->maxOnHook))){
       		pPulseDialData->currentPulseDigit++;
			
		}
		else {
			
			return 1;
		}
		getTime(pProTimer,pPulseDialData->offHookTime); 
	}
	
	return 0; 
}
int ProSLIC_DialPulseDetectTimeout (proslicChanType *pProslic, pulseDial_Cfg *pPulsedialCfg, pulseDialType *pPulseDialData){
	/*Pulse dial detect handling code start*/
		uInt8 HkStat;
		int time; uInt8 digit=0;
		ProSLIC_ReadHookStatus(pProslic,&HkStat);
		if (HkStat == ONHOOK){
			TimeElapsed(pProTimer,pPulseDialData->onHookTime,&time);
			if (time >  pPulsedialCfg->maxOnHook){
					
				return ON_HOOK_TIMEOUT;
			}
			    
		}
		if (HkStat == OFFHOOK && pPulseDialData->currentPulseDigit > 0){
			TimeElapsed(pProTimer,pPulseDialData->offHookTime,&time);
			if(time > pPulsedialCfg->maxOffHook){
				digit = pPulseDialData->currentPulseDigit;
				pPulseDialData->currentPulseDigit = 0;
					
				return digit;
			}
		}
		/*Pulse dial detect handling code end*/
		return 0;
}

int ProSLIC_DTMFReadDigit (proslicChanType_ptr hProslic,uInt8 *pDigit){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_DTMFReadDigit (hProslic,pDigit);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_DTMFReadDigit (hProslic,pDigit);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_DTMFReadDigit (hProslic,pDigit);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_DTMFReadDigit (hProslic,pDigit);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_DTMFReadDigit(hProslic,pDigit);
#endif
	return 1;
}

int ProSLIC_PLLFreeRunStart (proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_PLLFreeRunStart (hProslic);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_PLLFreeRunStart (hProslic);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_PLLFreeRunStart (hProslic);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_PLLFreeRunStart (hProslic);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_PLLFreeRunStart(hProslic);
#endif
	return 1;
}

int ProSLIC_PLLFreeRunStop (proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_PLLFreeRunStop (hProslic);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_PLLFreeRunStop (hProslic);
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_PLLFreeRunStop (hProslic);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_PLLFreeRunStop (hProslic);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_PLLFreeRunStop(hProslic);
#endif
	return 1;
}

int ProSLIC_PulseMeterStart (proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_PulseMeterStart (hProslic);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return 1;
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_PulseMeterStart (hProslic);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_PulseMeterStart (hProslic);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_PulseMeterStart(hProslic);
#endif
	return 1;
}

int ProSLIC_PulseMeterStop (proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return Si324x_PulseMeterStop (hProslic);
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return 1;
#endif
#ifdef SI321X
	if (hProslic->deviceId->chipType >= SI3210 && hProslic->deviceId->chipType <= SI3216M)
		return Si321x_PulseMeterStop (hProslic);
#endif
#ifdef SI3220
	if (hProslic->deviceId->chipType >= SI3220 && hProslic->deviceId->chipType <= SI3225)
		return Si3220_PulseMeterStop (hProslic);
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_PulseMeterStop(hProslic);
#endif
	return 1;
}

int ProSLIC_PowerUpConverter (proslicChanType_ptr hProslic){
#ifdef SI324X
	if (hProslic->deviceId->chipType >= SI3240 && hProslic->deviceId->chipType <= SI3247)
		return 1;
#endif
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_PowerUpConverter(hProslic);;
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_PowerUpConverter(hProslic);
#endif
	return 1;
}

int ProSLIC_PowerDownConverter (proslicChanType_ptr hProslic){
#ifdef SI32267
	if (hProslic->deviceId->chipType >= SI3226 && hProslic->deviceId->chipType <= SI3227)
		return Si3226_PowerDownConverter(hProslic);;
#endif
#ifdef SI3217X
	if (hProslic->deviceId->chipType >= SI32171 && hProslic->deviceId->chipType <= SI32179)
		return Si3217x_PowerDownConverter(hProslic);
#endif
	return 1;
}
int ProSLIC_LBCal (proslicChanType_ptr *pProslic, int size){
#ifdef SI32267
	if (pProslic[0]->deviceId->chipType >= SI3226 && pProslic[0]->deviceId->chipType <= SI3227)
		return Si3226_LBCal (pProslic,size);
#endif
#ifdef SI3217X
	if (pProslic[0]->deviceId->chipType >= SI32171 && pProslic[0]->deviceId->chipType <= SI32179)
		return Si3217x_LBCal(pProslic,size);
#endif
	return 1;
}

int ProSLIC_LBCalBroadcast (proslicChanType *pProslic){
#ifdef SI324X
	if (pProslic->deviceId->chipType >= SI3240 && pProslic->deviceId->chipType <= SI3247)
		return Si324x_LBCalBroadcast (pProslic);
#endif
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		return Si3226_LBCalBroadcast (pProslic);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		return Si3217x_LBCalBroadcast (pProslic);
#endif
	return 1;
}

int ProSLIC_GetLBCalResult (proslicChanType *pProslic,int32 *result1,int32 *result2, int32 *result3, int32 *result4){
#ifdef SI324X
	if (pProslic->deviceId->chipType >= SI3240 && pProslic->deviceId->chipType <= SI3247)
		return Si324x_GetLBCalResult (pProslic, result1,result2,result3,result4);
#endif
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		return Si3226_GetLBCalResult (pProslic,result1,result2,result3,result4);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		return Si3217x_GetLBCalResult (pProslic,result1,result2,result3,result4);
#endif
	return 1;
}

int ProSLIC_GetLBCalResultPacked (proslicChanType *pProslic,int32 *result){
#ifdef SI324X
	if (pProslic->deviceId->chipType >= SI3240 && pProslic->deviceId->chipType <= SI3247)
		return Si324x_GetLBCalResultPacked (pProslic, result);
#endif
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		return Si3226_GetLBCalResultPacked (pProslic,result);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		return Si3217x_GetLBCalResultPacked (pProslic,result);
#endif
	return 1;
}

int ProSLIC_LoadPreviousLBCal (proslicChanType *pProslic,int32 result1,int32 result2,int32 result3,int32 result4){
#ifdef SI324X
	if (pProslic->deviceId->chipType >= SI3240 && pProslic->deviceId->chipType <= SI3247)
		return Si324x_LoadPreviousLBCal (pProslic,result1,result2,result3,result4);
#endif
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		return Si3226_LoadPreviousLBCal (pProslic,result1,result2,result3,result4);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		return Si3217x_LoadPreviousLBCal (pProslic,result1,result2,result3,result4);
#endif
	return 1;
}

int ProSLIC_LoadPreviousLBCalPacked (proslicChanType *pProslic,int32 *result){
#ifdef SI324X
	if (pProslic->deviceId->chipType >= SI3240 && pProslic->deviceId->chipType <= SI3247)
		return Si324x_LoadPreviousLBCalPacked (pProslic,result);
#endif
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		return Si3226_LoadPreviousLBCalPacked (pProslic,result);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		return Si3217x_LoadPreviousLBCalPacked (pProslic,result);
#endif
	return 1;
}

int ProSLIC_dbgSetDCFeedVopen (proslicChanType *pProslic, uInt32 v_vlim_val,int32 preset){
#ifdef SI321X
	if (pProslic->deviceId->chipType >= SI3210 && pProslic->deviceId->chipType <= SI3216M)
		return Si321x_dbgSetDCFeedVopen(pProslic,v_vlim_val,preset);
#endif
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		return Si3226_dbgSetDCFeedVopen (pProslic,v_vlim_val,preset);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		return Si3217x_dbgSetDCFeedVopen (pProslic,v_vlim_val,preset);
#endif
	return 1;
}



int ProSLIC_dbgSetDCFeedIloop (proslicChanType *pProslic, uInt32 i_ilim_val, int32 preset){
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		return Si3226_dbgSetDCFeedIloop (pProslic,i_ilim_val,preset);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		return Si3217x_dbgSetDCFeedIloop (pProslic,i_ilim_val,preset);
#endif
#ifdef SI321X
	if (pProslic->deviceId->chipType >= SI3210 && pProslic->deviceId->chipType <= SI3216M)
		return Si321x_dbgSetDCFeedIloop(pProslic,i_ilim_val,preset);
#endif
	return 1;
}


int ProSLIC_dbgSetRinging (proslicChanType *pProslic, ProSLIC_dbgRingCfg *ringCfg, int preset){
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		return Si3226_dbgSetRinging (pProslic,ringCfg,preset);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		return Si3217x_dbgSetRinging (pProslic,ringCfg,preset);
#endif
	return 1;
}

int ProSLIC_dbgSetRXGain (proslicChanType *pProslic, int32 gain, int impedance_preset, int audio_gain_preset){
#ifdef SI321X
	if (pProslic->deviceId->chipType >= SI3210 && pProslic->deviceId->chipType <= SI3216M)
		return Si321x_dbgSetGain(pProslic,gain,impedance_preset,audio_gain_preset);
#endif
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		return Si3226_dbgSetRXGain (pProslic,gain,impedance_preset,audio_gain_preset);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		return Si3217x_dbgSetRXGain (pProslic,gain,impedance_preset,audio_gain_preset);
#endif
#ifdef SI324X
	if (pProslic->deviceId->chipType >= SI3240 && pProslic->deviceId->chipType <= SI3247)
		return Si324x_dbgSetRXGain(pProslic,gain,impedance_preset,audio_gain_preset);
#endif
	return 1;
}

int ProSLIC_dbgSetTXGain (proslicChanType *pProslic, int32 gain, int impedance_preset, int audio_gain_preset){

#ifdef SI321X
	if (pProslic->deviceId->chipType >= SI3210 && pProslic->deviceId->chipType <= SI3216M)
		return Si321x_dbgSetGain(pProslic,gain,impedance_preset,audio_gain_preset);
#endif
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		return Si3226_dbgSetTXGain (pProslic,gain,impedance_preset,audio_gain_preset);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		return Si3217x_dbgSetTXGain (pProslic,gain,impedance_preset,audio_gain_preset);
#endif
#ifdef SI324X
	if (pProslic->deviceId->chipType >= SI3240 && pProslic->deviceId->chipType <= SI3247)
		return Si324x_dbgSetTXGain(pProslic,gain,impedance_preset,audio_gain_preset);
#endif
	return 1;
}


/*
** Function: ProSLIC_LineMonitor
**
** Description: 
** Generic monitoring function
**
** Returns:
** 0
*/
int ProSLIC_LineMonitor (proslicChanType *pProslic, proslicMonitorType *monitor){
#ifdef SI324X
	if (pProslic->deviceId->chipType >= SI3240 && pProslic->deviceId->chipType <= SI3247)
		return Si324x_LineMonitor (pProslic,monitor);
#endif
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		return Si3226_LineMonitor (pProslic,monitor);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		return Si3217x_LineMonitor (pProslic, monitor);
#endif
	return 1;
}



/*
** Function: ProSLIC_PSTNCheck
**
** Description: 
** Monitor for excessive longitudinal current, which
** would be present if a live pstn line was connected
** to the port.
**
** Returns:
** 0 - no pstn detected
** 1 - pstn detected
*/
int ProSLIC_PSTNCheck (proslicChanType *pProslic, proslicPSTNCheckObjType *pPSTNCheck){
#ifdef SI32267
    return Si3226_PSTNCheck (pProslic,pPSTNCheck);
#endif
#ifdef SI3217X
    return Si3217x_PSTNCheck (pProslic,pPSTNCheck);
#endif
    return 1;
}


/*
** Function: ProSLIC_CreatePSTNCheckObj
**
** Description: 
** Allocate memory for pstnCheckObj
**
** Returns:
** 0
** -1 if malloc disabled
*/
int ProSLIC_CreatePSTNCheckObj(proslicPSTNCheckObjType_ptr *pstnCheckObj)
{
#ifndef DISABLE_MALLOC
#if 1
	*pstnCheckObj = kmalloc(sizeof(proslicPSTNCheckObjType), 0x20); //GFP_ATOMIC
	memset (*pstnCheckObj,0,sizeof(proslicPSTNCheckObjType));
    return 0;
#else
  	*pstnCheckObj = malloc(sizeof(proslicPSTNCheckObjType));
	memset (*pstnCheckObj,0,sizeof(proslicPSTNCheckObjType));
    return 0;
#endif
#else
	return -1;
#endif
}

/*
** Function: ProSLIC_InitPSTNCheckObj
**
** Description: 
** Initialize pstnCheckObj structure memebers
**
** Returns:
** 0
*/
int ProSLIC_InitPSTNCheckObj(proslicPSTNCheckObjType_ptr pstnCheckObj, int32 avgThresh, int32 singleThresh, uInt8 samples)
{
    pstnCheckObj->avgThresh = avgThresh;
    pstnCheckObj->singleThresh = singleThresh;
    pstnCheckObj->samples = samples;
    pstnCheckObj->avgIlong = 0;
    pstnCheckObj->count = 0;
    pstnCheckObj->buffFull = 0;
    
    return 0;
}


/*
** Function: ProSLIC_Version
**
** Description: 
** Return API version
**
** Returns:
** 0
*/
extern const char *proslicAPIVersion;
char *ProSLIC_Version()
{
	return (char *)proslicAPIVersion;
}

/** Add By Thlin **/

int ProSLIC_GetLinefeedStatus (proslicChanType *pProslic,uInt8 *newLinefeed)
{
#ifdef SI324X
    if (pProslic->deviceId->chipType >= SI3240 && pProslic->deviceId->chipType <= SI3247)
        return Si324x_GetLinefeedStatus (pProslic, newLinefeed);
#endif
#ifdef SI32267
    if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
        return Si3226_GetLinefeedStatus (pProslic, newLinefeed);
#endif
#ifdef SI3217X
    if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
        return Si3217x_GetLinefeedStatus (pProslic, newLinefeed);
#endif
    return 1;
}

int ProSLIC_Set_Ring_Cadence_ON(proslicChanType *pProslic, unsigned short msec){
#ifdef SI324X
	if (pProslic->deviceId->chipType >= SI3240 && pProslic->deviceId->chipType <= SI3247)
		Si324x_Set_Ring_Cadence_ON(pProslic, msec);
#endif
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		Si3226_Set_Ring_Cadence_ON(pProslic, msec);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		Si3217x_Set_Ring_Cadence_ON(pProslic, msec);
#endif
	return 1;

}

int ProSLIC_Set_Ring_Cadence_OFF(proslicChanType *pProslic, unsigned short msec){
#ifdef SI324X
	if (pProslic->deviceId->chipType >= SI3240 && pProslic->deviceId->chipType <= SI3247)
		Si324x_Set_Ring_Cadence_OFF(pProslic, msec);
#endif
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		Si3226_Set_Ring_Cadence_OFF(pProslic, msec);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		Si3217x_Set_Ring_Cadence_OFF(pProslic, msec);
#endif
	return 1;
}

int ProSLIC_Set_Impendance_Silicon(proslicChanType *pProslic, unsigned short country, unsigned short impd /*reserve*/){
#ifdef SI324X
	if (pProslic->deviceId->chipType >= SI3240 && pProslic->deviceId->chipType <= SI3247)
		Si324x_Set_Impendance_Silicon(pProslic, country, impd);
#endif
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		Si3226_Set_Impendance_Silicon(pProslic, country, impd);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		Si3217x_Set_Impendance_Silicon(pProslic, country, impd);
#endif
	return 1;
}

int ProSLIC_Set_Impendance(proslicChanType *pProslic, unsigned short preset){
#ifdef SI324X
	printk("NOT support @ %s, %s, line-%d\n", __FILE__, __FUNCTION__, __LINE__);
#endif
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		Si3226_Set_Impendance(pProslic, preset);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		Si3217x_Set_Impendance(pProslic, preset);
#endif
	return 1;
}

int ProSLIC_SendNTTCAR(proslicChanType *pProslic){
#ifdef SI324X
	if (pProslic->deviceId->chipType >= SI3240 && pProslic->deviceId->chipType <= SI3247)
		Si324x_SendNTTCAR(pProslic);
#endif
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		Si3226_SendNTTCAR(pProslic);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		Si3217x_SendNTTCAR(pProslic);
#endif
	return 1;
}

unsigned int ProSLIC_SendNTTCAR_check(unsigned int chid, proslicChanType *pProslic, unsigned long time_out){
#ifdef SI324X
	if (pProslic->deviceId->chipType >= SI3240 && pProslic->deviceId->chipType <= SI3247)
		return Si324x_SendNTTCAR_check(chid, pProslic, time_out);
#endif
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		return Si3226_SendNTTCAR_check(chid, pProslic, time_out);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		return Si3217x_SendNTTCAR_check(chid, pProslic, time_out);
#endif
	return 1;
}

int ProSLIC_SetUserMode(proslicChanType *pProslic, int on){
#ifdef SI324X
	if (pProslic->deviceId->chipType >= SI3240 && pProslic->deviceId->chipType <= SI3247)
		return SI324x_SetUserMode(pProslic, on);
#endif
#ifdef SI32267
	if (pProslic->deviceId->chipType >= SI3226 && pProslic->deviceId->chipType <= SI3227)
		return SI3226_SetUserMode(pProslic, on);
#endif
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32171 && pProslic->deviceId->chipType <= SI32179)
		return SI3217x_SetUserMode(pProslic, on);
#endif
	return 1;
}

int ProSLIC_SO_DTRx_Loopback (proslicChanType *pProslic, unsigned int enable)
{
#ifdef SI3217X
	if (pProslic->deviceId->chipType >= SI32177 && pProslic->deviceId->chipType <= SI32178)
	{
		SI3217X_Set_SO_DTx_Loopback(pProslic, enable);
	}
	else
		printk("This function only support SLIC Si32177/8\n");
#else
	printk("This function only support SLIC Si32177/8\n");
#endif
	return 1;
}


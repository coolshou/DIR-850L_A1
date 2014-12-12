/*
** Copyright (c) 2007-2010 by Silicon Laboratories
**
** $Id: si3226x_intf.c 1030 2009-09-01 22:59:44Z nizajerk@SILABS.COM $
**
** SI3226X_Intf.c
** SI3226X ProSLIC interface implementation file
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
** This is the implementation file for the main ProSLIC API and is used 
** in the ProSLIC demonstration code. 
**
*/

#include "si_voice_datatypes.h"
#include "si_voice_ctrl.h"
#include "si_voice_timer_intf.h"
#include "proslic.h"
#include "si3226x_intf.h"
#include "si3226x.h"
#include "si3226x_registers.h"
#include "proslic_api_config.h"

#define PRAM_ADDR (334 + 0x400)
#define PRAM_DATA (335 + 0x400)

#define WriteReg        pProslic->deviceId->ctrlInterface->WriteRegister_fptr
#define ReadReg         pProslic->deviceId->ctrlInterface->ReadRegister_fptr
#define pProHW          pProslic->deviceId->ctrlInterface->hCtrl
#define Reset           pProslic->deviceId->ctrlInterface->Reset_fptr
#define Delay           pProslic->deviceId->ctrlInterface->Delay_fptr
#define pProTimer       pProslic->deviceId->ctrlInterface->hTimer
#define WriteRAM        pProslic->deviceId->ctrlInterface->WriteRAM_fptr
#define ReadRAM         pProslic->deviceId->ctrlInterface->ReadRAM_fptr
#define TimeElapsed pProslic->deviceId->ctrlInterface->timeElapsed_fptr
#define getTime      pProslic->deviceId->ctrlInterface->getTime_fptr
#define SetSemaphore pProslic->deviceId->ctrlInterface->Semaphore_fptr

#define WriteRegX               deviceId->ctrlInterface->WriteRegister_fptr
#define ReadRegX                deviceId->ctrlInterface->ReadRegister_fptr
#define pProHWX                 deviceId->ctrlInterface->hCtrl
#define DelayX                  deviceId->ctrlInterface->Delay_fptr
#define pProTimerX              deviceId->ctrlInterface->hTimer
#define WriteRAMX               deviceId->ctrlInterface->WriteRAM_fptr
#define ReadRAMX                deviceId->ctrlInterface->ReadRAM_fptr
#define getTimeX             deviceId->ctrlInterface->getTime_fptr
#define TimeElapsedX    deviceId->ctrlInterface->timeElapsed_fptr

#define BROADCAST 0xff

/*
** Externs
*/

/* General Configuration */
extern Si3226x_General_Cfg Si3226x_General_Configuration;
#ifdef SIVOICE_MULTI_BOM_SUPPORT
extern const proslicPatch SI3226X_PATCH_B_FLBK;
extern const proslicPatch SI3226X_PATCH_B_FIXRL;
extern const proslicPatch SI3226X_PATCH_C_FLBK;
extern const proslicPatch SI3226X_PATCH_C_QCUK;
extern const proslicPatch SI3226X_PATCH_C_TSS;
extern const proslicPatch SI3226X_PATCH_C_TSS_ISO;
extern Si3226x_General_Cfg Si3226x_General_Configuration_MultiBOM[];
extern int si3226x_genconf_multi_max_preset;
#else
extern const proslicPatch SI3226X_PATCH_B_DEFAULT;
extern const proslicPatch SI3226X_PATCH_C_DEFAULT;
#endif

/* Ringing */
#ifndef DISABLE_RING_SETUP
extern Si3226x_Ring_Cfg Si3226x_Ring_Presets[];
#endif

/* Tone Generation */
#ifndef DISABLE_TONE_SETUP
extern Si3226x_Tone_Cfg Si3226x_Tone_Presets[];
#endif

/* FSK */
#ifndef DISABLE_FSK_SETUP
extern Si3226x_FSK_Cfg Si3226x_FSK_Presets[];
#endif

/* DTMF */
#ifndef DISABLE_DTMF_SETUP
extern Si3226x_DTMFDec_Cfg Si3226x_DTMFDec_Presets[];
#endif

/* Zsynth */
#ifndef DISABLE_ZSYNTH_SETUP
extern Si3226x_Impedance_Cfg Si3226x_Impedance_Presets [];
#endif

/* CI/GCI */
#ifndef DISABLE_CI_SETUP
extern Si3226x_CI_Cfg Si3226x_CI_Presets [];
#endif

/* Audio Gain Scratch */
extern Si3226x_audioGain_Cfg Si3226x_audioGain_Presets[];

/* DC Feed */
#ifndef DISABLE_DCFEED_SETUP
extern Si3226x_DCfeed_Cfg Si3226x_DCfeed_Presets[];
#endif

/* GPIO */
#ifndef DISABLE_GPIO_SETUP
extern Si3226x_GPIO_Cfg Si3226x_GPIO_Configuration ;
#endif

/* Pulse Metering */
#ifndef DISABLE_PULSE_SETUP
extern Si3226x_PulseMeter_Cfg Si3226x_PulseMeter_Presets [];
#endif

/* PCM */
#ifndef DISABLE_PCM_SETUP
extern Si3226x_PCM_Cfg Si3226x_PCM_Presets [];
#endif



/*
** Local functions are defined first
*/

/*
** Function: getChipType
**
** Description: 
** Decode ID register to identify chip type
**
** Input Parameters: 
** ID register value
**
** Return:
** partNumberType
*/
static partNumberType getChipType(uInt8 data){
    data &= 0x38;
    return ((data >> 3) + SI32260);
}

/*
** Function: setUserMode
**
** Description: 
** Puts ProSLIC into user mode or out of user mode
**
** Input Parameters: 
** pProslic: pointer to PROSLIC object
** on: specifies whether user mode should be turned on (TRUE) or off (FALSE)
**
** Return:
** none
*/
static int setUserMode (proslicChanType *pProslic,BOOLEAN on){
    uInt8 data;
    if (SetSemaphore != NULL){
        while (!(SetSemaphore (pProHW,1)));
        if (on == TRUE){
            if (pProslic->deviceId->usermodeStatus<2)
                pProslic->deviceId->usermodeStatus++;
        } else {
            if (pProslic->deviceId->usermodeStatus>0)
                pProslic->deviceId->usermodeStatus--;
            if (pProslic->deviceId->usermodeStatus != 0)
                return -1;
        }
    }
    data = ReadReg(pProHW,pProslic->channel,USERMODE_ENABLE);
    if (((data&1) != 0) == on)
        return 0;
    WriteReg(pProHW,pProslic->channel,USERMODE_ENABLE,2);
    WriteReg(pProHW,pProslic->channel,USERMODE_ENABLE,8);
    WriteReg(pProHW,pProslic->channel,USERMODE_ENABLE,0xe);
    WriteReg(pProHW,pProslic->channel,USERMODE_ENABLE,0);
    if (SetSemaphore != NULL)
        SetSemaphore(pProHW,0);
    return 0;
}


/*
** Function: setUserModeBroadcast
**
** Description: 
** Puts ProSLIC into user mode or out of user mode
**
** Input Parameters: 
** pProslic: pointer to PROSLIC object
** on: specifies whether user mode should be turned on (TRUE) or off (FALSE)
**
** Return:
** none
*/
static int setUserModeBroadcast (proslicChanType *pProslic,BOOLEAN on){
    uInt8 data;
    if (SetSemaphore != NULL){
        while (!(SetSemaphore (pProHW,1)));
        if (on == TRUE){
            if (pProslic->deviceId->usermodeStatus<2)
                pProslic->deviceId->usermodeStatus++;
        } else {
            if (pProslic->deviceId->usermodeStatus>0)
                pProslic->deviceId->usermodeStatus--;
            if (pProslic->deviceId->usermodeStatus != 0)
                return -1;
        }
    }
    data = ReadReg(pProHW,pProslic->channel,USERMODE_ENABLE);/*we check first channel. we assume all channels same user mode state*/
    if (((data&1) != 0) == on)
        return 0;
    WriteReg(pProHW,BROADCAST,USERMODE_ENABLE,2);
    WriteReg(pProHW,BROADCAST,USERMODE_ENABLE,8);
    WriteReg(pProHW,BROADCAST,USERMODE_ENABLE,0xe);
    WriteReg(pProHW,BROADCAST,USERMODE_ENABLE,0);
    if (SetSemaphore != NULL)
        SetSemaphore(pProHW,0);
    return 0;
}


/*
** Function: isVerifiedProslic
**
** Description: 
** Determine if DAA or ProSLIC present
**
** Input Parameters: 
** pProslic: pointer to PROSLIC channel object
**
** Return:
** channelType
*/
static int identifyChannelType(proslicChanType *pProslic){
uInt8 data;
    /*
    **  Register 13 (DAA) always has bits 0:1 set to 0 and bit 6 set to 1
    **  Register 13 (PROSLIC) can have bits 0:1, and 4 set, while all others are undefined
    **  Write 0x13 to Reg 13. The following return values are expected -
    **
    **   0x00 or 0xFF    :    No device present
    **   0x4X            :    DAA
    **   0x13            :    PROSLIC
    */

WriteReg(pProHW,pProslic->channel,PCMTXHI,0x13);
    data = ReadReg(pProHW,pProslic->channel,PCMTXHI); /* Active delay */
    data = ReadReg(pProHW,pProslic->channel,PCMTXHI); /* Read again */
    if( data == 0x13)
       return PROSLIC;
	else if (data == 0x40)
       return DAA;
    else
       return UNKNOWN;
}







/*
** Function: Si3226x_EnablePatch
**
** Description: 
** Enables patch
**
** Input Parameters:
** pProslic:   pointer to PROSLIC channel obj
**
** Returns:
** 0
*/
static int Si3226x_EnablePatch (proslicChanType *pProslic){ 
    setUserMode (pProslic,TRUE);
    WriteReg (pProHW, pProslic->channel, JMPEN,1); 
    setUserMode (pProslic,FALSE);
    return 0;
}


/*
** Function: Si3226x_PowerUpConverter
**
** Description: 
** Powers all DC/DC converters sequentially with delay to minimize
** peak power draw on VDC.
**
** Returns:
** int (error)
**
*/
int Si3226x_PowerUpConverter(proslicChanType_ptr pProslic)
{
    errorCodeType error = RC_NONE;
    int32 vbath,vbat;
    uInt8 reg;
    ramData data;
    int timer = 0;

    setUserMode(pProslic,TRUE);

    /*
    ** Check to see if already powered, return if so
    */
    data = ReadRAM(pProHW,pProslic->channel,PD_DCDC);
    if(!(data & 0x100000)) 
    {
        setUserMode(pProslic,FALSE);
        return error;   /* Return if already powered up */
    }

    /*
    ** Power up sequence 
    */
    if(Si3226x_General_Configuration.batType == BO_DCDC_TRACKING)
        {
        /*
        ** TRACKING CONVERTER SEQUENCE
        **
        ** - clear previous ov/uv lockout
        ** - powerup charge pump
        ** - delay
        ** - powerup digital dc/dc w/ OV clamping and shutdown
        ** - delay
        ** - verify no short circuits by looking for vbath/2
        ** - clear dcdc status
        ** - switch to analog converter with OV clamping only (no shutdown)
        ** - select analog dcdc and disable pwrsave
        ** - delay
        */

        WriteRAM(pProHW,pProslic->channel,DCDC_OITHRESH,Si3226x_General_Configuration.dcdc_oithresh_lo);    
        WriteReg(pProHW,pProslic->channel,LINEFEED,LF_OPEN);  /* Ensure open before powering converter */
        reg = ReadReg(pProHW,pProslic->channel,ENHANCE);      /* Read ENHANCE entry settings */
        WriteReg(pProHW,pProslic->channel,ENHANCE,reg&0x07);  /* Disable powersave during bringup */
        WriteRAM(pProHW,pProslic->channel,PD_DCDC,0x700000L);   /* In case OV or UV previously occurred */
        WriteRAM(pProHW,pProslic->channel,DCDC_CPUMP,0x100000L);/* Turn on charge pump */
        Delay(pProTimer,10);
        WriteRAM(pProHW,pProslic->channel,PD_DCDC,0x600000L);
        Delay(pProTimer,50);
        vbath = ReadRAM(pProHW,pProslic->channel,VBATH_EXPECT);
        vbat = ReadRAM(pProHW,pProslic->channel,MADC_VBAT);
        if(vbat & 0x10000000L)
            vbat |= 0xF0000000L;
#ifdef ENABLE_DEBUG
        if(pProslic->debugMode)
        {
            LOGPRINT ("VBAT @ 50ms = %d.%d v\n",(int)((vbat/SCALE_V_MADC)/1000), (int)(((vbat/SCALE_V_MADC) - (vbat/SCALE_V_MADC)/1000*1000)));
        }
#endif
        if(vbat < (vbath / 2)) {
            pProslic->channelEnable = 0;
            error = RC_VBAT_UP_TIMEOUT;
            WriteRAM(pProHW,pProslic->channel,PD_DCDC, 0x300000L); /* shutdown converter */
            #ifdef ENABLE_DEBUG
            if(pProslic->debugMode)
            {
                LOGPRINT ("Si3226x DCDC Short Circuit Failure %d - disabling channel\n",pProslic->channel);
            }
            #endif      
            setUserMode(pProslic,FALSE);
            return error;  
        }
        else { /* Enable analog converter */
            WriteRAM(pProHW,pProslic->channel,DCDC_STATUS,0L);   
            WriteRAM(pProHW,pProslic->channel,PD_DCDC,0x400000L);
            WriteReg(pProHW,pProslic->channel,ENHANCE,reg);   /* restore ENHANCE setting */
            Delay(pProTimer,50);
        }

        /*
        ** - monitor vbat vs expected level (VBATH_EXPECT)
        */
        vbath = ReadRAM(pProHW,pProslic->channel,VBATH_EXPECT);
        do
        {
            vbat = ReadRAM(pProHW,pProslic->channel,MADC_VBAT);
            if(vbat & 0x10000000L)
                vbat |= 0xF0000000L;
            Delay(pProTimer,10);
        }while((vbat < (vbath - COMP_5V))&&(timer++ < 200));  /* 2 sec timeout */

    #ifdef ENABLE_DEBUG
        if(pProslic->debugMode)
        {
            LOGPRINT ("VBAT Up = %d.%d v\n",(int)((vbat/SCALE_V_MADC)/1000), (int)(((vbat/SCALE_V_MADC) - (vbat/SCALE_V_MADC)/1000*1000)));
        }
    #endif
        if(timer > 200)
        {
            /* Error handling - shutdown converter, disable channel, set error tag */
            pProslic->channelEnable = 0;
            error = RC_VBAT_UP_TIMEOUT;
            WriteRAM(pProHW,pProslic->channel,PD_DCDC, 0x300000L); /* shutdown converter */
    #ifdef ENABLE_DEBUG
            if(pProslic->debugMode)
            {
                LOGPRINT ("Si3226x DCDC Power up timeout channel %d - disabling channel\n",pProslic->channel);
            }
    #endif      
        }
        setUserMode(pProslic,FALSE);
        return error;  
    }
    else if((Si3226x_General_Configuration.batType == BO_DCDC_TSS)||(Si3226x_General_Configuration.batType == BO_DCDC_TSS_ISO))
    {
        /*
        ** FIXED RAIL CONVERTER SEQUENCE
        **
        ** - return if even channel
        ** - clear previous ov/uv lockout
        ** - powerup charge pump
        ** - delay
        ** - powerup converter
        ** - delay
        ** - verify no short circuits by looking for vbath/2
        ** - clear dcdc status
        ** - delay
        */

        if( pProslic->channel %2 == 0)  /* is even */
        {
#ifdef ENABLE_DEBUG
            if(pProslic->debugMode)
            {
                LOGPRINT("si3226x : DCDC Powerup Channel %d Ignored\n", pProslic->channel);
            }
#endif
            setUserMode(pProslic,FALSE);
            return RC_IGNORE;
        }

        WriteRAM(pProHW,pProslic->channel,DCDC_OITHRESH,Si3226x_General_Configuration.dcdc_oithresh_lo);   
        WriteReg(pProHW,pProslic->channel,LINEFEED,LF_OPEN);  /* Ensure open before powering converter */
        reg = ReadReg(pProHW,pProslic->channel,ENHANCE);      /* Read ENHANCE entry settings */
        WriteReg(pProHW,pProslic->channel,ENHANCE,reg&0x07);  /* Disable powersave during bringup */
        WriteRAM(pProHW,pProslic->channel,PD_DCDC,0x700000L);   /* In case OV or UV previously occurred */

        /* Do not turn on charge pump if isolated design */
        if(Si3226x_General_Configuration.batType != BO_DCDC_TSS_ISO)
        {
            WriteRAM(pProHW,pProslic->channel,DCDC_CPUMP,0x100000L);/* Turn on charge pump */
        }

        Delay(pProTimer,10);
        WriteRAM(pProHW,pProslic->channel,PD_DCDC,0x600000L);
        Delay(pProTimer,500);
        vbath = ReadRAM(pProHW,pProslic->channel,VBATH_EXPECT);
        vbat = ReadRAM(pProHW,pProslic->channel,MADC_VBAT);
        if(vbat & 0x10000000L)
            vbat |= 0xF0000000L;
        if(vbath & 0x10000000L)
            vbath |= 0xF0000000L;

            LOGPRINT ("Initial VBAT = %d.%d v\n",(int)((vbat/SCALE_V_MADC)/1000), (int)(((vbat/SCALE_V_MADC) - (vbat/SCALE_V_MADC)/1000*1000)));
 
       if(vbat < (vbath / 2)) {
            pProslic->channelEnable = 0;
            error = RC_VBAT_UP_TIMEOUT;
            WriteRAM(pProHW,pProslic->channel,PD_DCDC, 0x300000L); /* shutdown converter */
            #ifdef ENABLE_DEBUG
            if(pProslic->debugMode)
            {
                LOGPRINT ("Si3226x DCDC Short Circuit Failure %d - disabling channel\n",pProslic->channel);
            }
            #endif  
            setUserMode(pProslic,FALSE);    
            return error;  
        }
        else {
            WriteRAM(pProHW,pProslic->channel,DCDC_STATUS,0L);   
            //WriteReg(pProHW,pProslic->channel,ENHANCE,reg);   /* restore ENHANCE setting */
            Delay(pProTimer,50);
        }

        /*
        ** - monitor vbat vs expected level (VBATH_EXPECT)
        */
        vbath = ReadRAM(pProHW,pProslic->channel,VBATH_EXPECT);
        do
        {
            vbat = ReadRAM(pProHW,pProslic->channel,MADC_VBAT);
            if(vbat & 0x10000000L)
                vbat |= 0xF0000000L;
            Delay(pProTimer,10);
        }while((vbat < (vbath - COMP_5V))&&(timer++ < 200));  /* 2 sec timeout */

    #ifdef ENABLE_DEBUG
        if(pProslic->debugMode)
        {
            LOGPRINT ("Fixed VBAT Up = %d.%d v\n",(int)((vbat/SCALE_V_MADC)/1000), (int)(((vbat/SCALE_V_MADC) - (vbat/SCALE_V_MADC)/1000*1000)));
        }
    #endif
        if(timer > 200)
        {
            /* Error handling - shutdown converter, disable channel, set error tag */
            pProslic->channelEnable = 0;
            error = RC_VBAT_UP_TIMEOUT;
            WriteRAM(pProHW,pProslic->channel,PD_DCDC, 0x300000L); /* shutdown converter */
    #ifdef ENABLE_DEBUG
            if(pProslic->debugMode)
            {
                LOGPRINT ("Si3226x DCDC Fixed Rail Power up timeout channel %d - disabling channel\n",pProslic->channel);
            }
    #endif      
        }

        /* Restore ENHANCE reg */
        WriteReg(pProHW,pProslic->channel,ENHANCE,reg);
        setUserMode(pProslic,FALSE); 
        return error;  
    }
    else /* external battery - just verify presence */
    {
        /*
        ** - monitor vbat vs expected level (VBATH_EXPECT)
        */
        vbath = ReadRAM(pProHW,pProslic->channel,VBATH_EXPECT);
        do
        {
            vbat = ReadRAM(pProHW,pProslic->channel,MADC_VBAT);
            if(vbat & 0x10000000L)
                vbat |= 0xF0000000L;
            Delay(pProTimer,10);
        }while((vbat < (vbath - COMP_5V))&&(timer++ < 200));  /* 2 sec timeout */

    #ifdef ENABLE_DEBUG
        if(pProslic->debugMode)
        {
            LOGPRINT ("Ext VBAT Up = %d.%d v\n",(int)((vbat/SCALE_V_MADC)/1000), (int)(((vbat/SCALE_V_MADC) - (vbat/SCALE_V_MADC)/1000*1000)));
        }
    #endif
        if(timer > 200)
        {
            /* Error handling - shutdown converter, disable channel, set error tag */
            pProslic->channelEnable = 0;
            error = RC_VBAT_UP_TIMEOUT;
    #ifdef ENABLE_DEBUG
            if(pProslic->debugMode)
            {
                LOGPRINT ("Si3226x External VBAT timeout channel %d - disabling channel\n",pProslic->channel);
            }
    #endif      
        }


    }

    setUserMode(pProslic,FALSE);
	return error;
}

/*
** Function: Si3226x_PowerDownConverter
**
** Description: 
** Safely powerdown dcdc converter after ensuring linefeed
** is in the open state.  Test powerdown by setting error
** flag if detected voltage does no fall below 5v.
**
** Returns:
** int (error)
**
*/
int Si3226x_PowerDownConverter(proslicChanType_ptr pProslic)
{
    errorCodeType error = RC_NONE;
    int32 vbat;
    int timer = 0;
    ramData data;

    setUserMode(pProslic,TRUE);
    /*
    ** Check to see if already powered down, return if so
    */
    data = ReadRAM(pProHW,pProslic->channel,PD_DCDC);
    if((data & 0x100000)) 
    {
        setUserMode(pProslic,FALSE);
        return error;   /* Return if already powered down */
    }

    /*
    ** Power down sequence */
    WriteReg(pProHW,pProslic->channel,LINEFEED,LF_FWD_OHT);  /* Force out of powersave mode */  
    WriteReg(pProHW,pProslic->channel,LINEFEED, LF_OPEN);
    Delay(pProTimer,50);
    WriteRAM(pProHW,pProslic->channel,PD_DCDC,0x900000L);
    Delay(pProTimer,50);

    /*
    ** Verify VBAT falls below 5v
    */
    do
    {
        vbat = ReadRAM(pProHW,pProslic->channel,MADC_VBAT);
        if(vbat & 0x10000000L)
            vbat |= 0xF0000000L;
        Delay(pProTimer,10);
    }while((vbat > COMP_5V)&&(timer++ < 20));  /* 200 msec timeout */
#ifdef ENABLE_DEBUG
    if(pProslic->debugMode)
    {
        LOGPRINT ("VBAT Down = %d.%d v\n",(int)((vbat/SCALE_V_MADC)/1000), (int)(((vbat/SCALE_V_MADC) - (vbat/SCALE_V_MADC)/1000*1000)));
    }
#endif
    if(timer > 20)
    {
        /* Error handling - shutdown converter, disable channel, set error tag */
        pProslic->channelEnable = 0;
        error = RC_VBAT_DOWN_TIMEOUT;
#ifdef ENABLE_DEBUG
        if(pProslic->debugMode)
        {
            LOGPRINT ("Si3226x DCDC Power Down timeout channel %d\n",pProslic->channel);
        }
#endif      
    }

    setUserMode(pProslic,FALSE);
    return error;  
}


/* Old cal left in for now until we clear up forward decs */
int Si3226x_Cal(proslicChanType_ptr *pProslic, int maxChan)
{
    return 0;
}

/*
** Function: Si3226x_Calibrate
**
** Description: 
** Performs calibration based on passed ptr to array of
** desired CALRn settings.
**
** Starts calibration on all channels sequentially (not broadcast)
** and continuously polls for completion.  Return error code if
** CAL_EN does not clear for each enabled channel within the passed
** timeout period.
*/
int Si3226x_Calibrate(proslicChanType_ptr *pProslic, int maxChan, uInt8 *calr, int maxTime)
{
    int i;
    int cal_en = 0;
    int cal_en_chan = 0;
    int timer = 0;

    /*
    ** Launch cals sequentially (not serially)
    */
    for(i=0;i<maxChan;i++)
    {
        if((pProslic[i]->channelEnable)&&(pProslic[i]->channelType == PROSLIC))
        {
            pProslic[i]->WriteRegX(pProslic[i]->pProHWX,pProslic[i]->channel,CALR0,calr[0]);
            pProslic[i]->WriteRegX(pProslic[i]->pProHWX,pProslic[i]->channel,CALR1,calr[1]);
            pProslic[i]->WriteRegX(pProslic[i]->pProHWX,pProslic[i]->channel,CALR2,calr[2]);
            pProslic[i]->WriteRegX(pProslic[i]->pProHWX,pProslic[i]->channel,CALR3,calr[3]);
        }
    }

    /*
    ** Wait for completion or timeout
    */
    do 
    {
        cal_en = 0;
        pProslic[0]->DelayX(pProslic[0]->pProTimerX,10);
        for(i=0;i<maxChan;i++)
        {
            if(pProslic[i]->channelEnable)
            {
                cal_en_chan = pProslic[i]->ReadRegX(pProslic[i]->pProHWX,pProslic[i]->channel,CALR3);
                if((cal_en_chan&0x80)&&(timer == maxTime))
                {
#ifdef ENABLE_DEBUG
                    if(pProslic[i]->debugMode)
                    {
                        LOGPRINT("Calibration timout channel %d\n",i);
                    }
#endif
                    pProslic[i]->channelEnable = 0;
                    pProslic[i]->error = RC_CAL_TIMEOUT;
                }
                cal_en |= cal_en_chan;
            }
        }         
    }while((timer++ <= maxTime)&&(cal_en&0x80));
    return cal_en;
}



/*
** Function: LoadRegTables
**
** Description: 
** Generic function to load register/RAM with predefined addr/value 
*/
static int LoadRegTables (proslicChanType *pProslic, ProslicRAMInit *pRamTable, ProslicRegInit *pRegTable, int broadcast){
    uInt16 i;
    uInt8 channel;
    if (broadcast){
        channel = BROADCAST;
        setUserModeBroadcast(pProslic,TRUE);
    }
    else {
        channel = pProslic->channel;
        setUserMode(pProslic,TRUE);
    }

    i=0; 
    if (pRamTable != 0){
        while (pRamTable[i].address != 0xffff){
            WriteRAM(pProHW,channel,pRamTable[i].address,pRamTable[i].initValue); 
            i++;
        }
    }
    i=0;
    if (pRegTable != 0){
        while (pRegTable[i].address != 0xff){
            WriteReg(pProHW,channel,pRegTable[i].address,pRegTable[i].initValue);
            i++;
        }
    }
    if (broadcast)
        setUserModeBroadcast(pProslic,FALSE);
    else
        setUserMode(pProslic,FALSE);

    return 0;
}



/*
** Function: LoadSi3226xPatch
**
** Description: 
** Load patch from external file defined as 'RevBPatch'
**
** Input Parameters: 
** pProslic: pointer to PROSLIC channel object
** broadcast:  broadcast flag
**
** Return:
** 0
*/
static int LoadSi3226xPatch (proslicChanType *pProslic, const proslicPatch *pPatch,int broadcast){ 
    int32 loop; 
    uInt8  jmp_table_low  = PATCH_JMPTBL_LOW_ADDR;
    uInt16 jmp_table_high = PATCH_JMPTBL_HIGH_ADDR;
    uInt8 channel;
    ramData data;

    if (pPatch == NULL)
        return 0;
    if (broadcast){
        setUserModeBroadcast(pProslic,TRUE);
        channel = BROADCAST;
    }
    else{
        setUserMode (pProslic,TRUE); /*make sure we are in user mode to load patch*/
        channel = pProslic->channel;
    }

    /* Disable patch */
    WriteReg (pProHW, pProslic->channel, JMPEN,0); 

    /**
     * Zero out jump table(s) in case previous values are still loaded
     */
    for (loop=0;loop<PATCH_NUM_LOW_ENTRIES;loop++){
        /*zero out the jump table*/
        WriteReg (pProHW, channel, jmp_table_low,0);
        WriteReg (pProHW, channel, jmp_table_low+1,0); 
        jmp_table_low+=2;
    }

    for (loop=0;loop<PATCH_NUM_HIGH_ENTRIES;loop++){
        /*zero out the jump table*/
        WriteRAM (pProHW, channel, jmp_table_high,0L);
        jmp_table_high++;
    }
    

    /**
     * Load patch RAM data
     */
    WriteRAM(pProHW, channel,PRAM_ADDR, 0); /*write patch ram address register
                                              If the data is all 0, you have hit the end of the programmed values and can stop loading.*/
    for (loop=0; loop<PATCH_MAX_SIZE; loop++){ 
        if (pPatch->patchData[loop] != 0){
            if ((pProslic->deviceId->chipRev < 3) && broadcast)
                WriteRAM(pProHW, channel,PRAM_ADDR, loop<<19); /*write patch ram address register (only necessary for broadcast rev c and earlier)*/
            WriteRAM(pProHW, channel,PRAM_DATA,pPatch->patchData[loop]<<9); /*loading patch, note. data is shifted*/
        }
        else
            loop = 1024;
    }

    /* Delay 1 mSec to ensure last RAM write completed - this should be quicker than doing a SPI access
       to confirm the status register.
     */
    Delay(pProHW, 1); 

    /*zero out RAM_ADDR_HI*/
    WriteReg (pProHW, channel, RAM_ADDR_HI,0);

    /**
     * Lower 8 Jump Table Entries - register space
     */
    jmp_table_low=PATCH_JMPTBL_LOW_ADDR;
    for (loop=0;loop<PATCH_NUM_LOW_ENTRIES;loop++){
        /* Load the jump table with the new values.*/
        if (pPatch->patchEntries[loop] != 0){
            WriteReg (pProHW, channel, jmp_table_low,(pPatch->patchEntries[loop])&0xff);
            WriteReg (pProHW, channel, jmp_table_low+1,pPatch->patchEntries[loop]>>8);
        }
        jmp_table_low+=2;
    }

    /**
     * Upper 8 Jump Table Entries - Memory Mapped register space 
     */
    jmp_table_high=PATCH_JMPTBL_HIGH_ADDR;
    for (loop=0;loop<PATCH_NUM_HIGH_ENTRIES;loop++){
        if (pPatch->patchEntries[loop] != 0)
        {
            data = ((uInt32) (pPatch->patchEntries[loop+PATCH_NUM_LOW_ENTRIES])) & 0x00001fffL ;
            WriteRAM (pProHW, channel, jmp_table_high, data );
        }
        jmp_table_high++;
    }

    WriteRAM(pProHW,channel,PATCH_ID,pPatch->patchSerial); /*write patch identifier*/

    /**
     * Write patch support RAM locations (if any) 
     */
    for (loop=0; loop<PATCH_MAX_SUPPORT_RAM; loop++){
        if(pPatch->psRamAddr[loop] != 0) {
            WriteRAM(pProHW,channel,pPatch->psRamAddr[loop],pPatch->psRamData[loop]);
        }
        else {
            loop = PATCH_MAX_SUPPORT_RAM;
        }
    }

    if (broadcast){
        setUserModeBroadcast(pProslic,FALSE);
    }
    else {
        setUserMode(pProslic,FALSE); /*turn off user mode*/
    }
    return 0;
}





/*
** Functions below are defined in header file and can be called by external files
*/

/*
**
** PROSLIC INITIALIZATION FUNCTIONS
**
*/

/*
** Function: PROSLIC_Reset
**
** Description: 
** Resets the ProSLIC
*/
int Si3226x_Reset (proslicChanType_ptr pProslic){
    /*
    ** resets ProSLIC, wait 250ms, release reset, wait 250ms
    */
    Reset(pProHW,1);
    Delay(pProTimer,250);
    Reset(pProHW,0);
    Delay(pProTimer,250);
    return 0;
}

/*
** Function: Si3226x_ShutdownChannel
**
** Description: 
** Safely shutdown channel w/o interruptions to
** other active channels
**
** Input Parameters: 
** pProslic: pointer to PROSLIC channel object
**
** Return:
** 0

*/
int Si3226x_ShutdownChannel (proslicChanType_ptr pProslic){
uInt8 reg;
int error = 0;
    /*
    ** set linefeed to open state, powerdown dcdc converter
    */
    reg = ReadReg(pProHW,pProslic->channel,LINEFEED);
    if(reg != 0)
        Si3226x_SetLinefeedStatus(pProslic,LF_FWD_OHT);  /* force low power mode exit */
    Si3226x_SetLinefeedStatus(pProslic,LF_OPEN);
    Delay(pProTimer,10);

    /* 
    ** Shutdown converter if not using external supply.  
    */
    if(Si3226x_General_Configuration.batType != BO_DCDC_EXTERNAL)
        error = Si3226x_PowerDownConverter(pProslic);
    
    return error;
}

/*
** Function: Si3226x_VerifyControlInterface
**
** Description: 
** Check control interface readback cababilities
**
** Input Parameters: 
** pProslic: pointer to PROSLIC channel object
**
** Return:
** 0
*/
int Si3226x_VerifyControlInterface (proslicChanType_ptr pProslic)
{
    if (identifyChannelType(pProslic) != PROSLIC)
        return RC_CHANNEL_TYPE_ERR;

    WriteReg (pProHW,pProslic->channel,PCMRXLO,0x5A);
    if (ReadReg(pProHW,pProslic->channel,PCMRXLO) != 0x5A){
        pProslic->error = RC_SPI_FAIL;
#ifdef ENABLE_DEBUG
        if (pProslic->debugMode)
            LOGPRINT("Si3226x: Proslic %d registers not communicating.\n",pProslic->channel);
#endif
        return RC_SPI_FAIL;
    }

    /* Verify RAM rd/wr with innocuous RAM location */
    WriteRAM(pProHW,pProslic->channel,UNUSED449,0x12345678L);
    if (ReadRAM(pProHW,pProslic->channel, UNUSED449) != 0x12345678L){
        pProslic->error = RC_SPI_FAIL;

#ifdef ENABLE_DEBUG
        if (pProslic->debugMode)
            LOGPRINT("Si3226x: Proslic %d RAM not communicating. RAM access fail.\n",pProslic->channel);
#endif
        return RC_SPI_FAIL;
    }
    return RC_NONE;
}


/*
** Function: Si3226x_Init_MultiBOM
**
** Description: 
** - probe SPI to establish daisy chain length
** - load patch
** - initialize general parameters
** - calibrate madc
** - bring up DC/DC converters
** - calibrate remaining items except madc & lb
**
** Input Parameters: 
** pProslic: pointer to PROSLIC object array
** fault: error code
**
** Return:
** error code
*/

#ifdef SIVOICE_MULTI_BOM_SUPPORT

int Si3226x_Init_MultiBOM(proslicChanType_ptr *pProslic,int size, int preset) {

	if(preset < si3226x_genconf_multi_max_preset)
	{
		/* Copy selected General COnfiguration parameters to std structure */
		Si3226x_General_Configuration = Si3226x_General_Configuration_MultiBOM[preset];
	}
	else
	{
		return RC_INVALID_PRESET;
	}
	return Si3226x_Init(pProslic,size);
}
#endif

/*
** Function: Si3226x_Init
**
** Description: 
** - probe SPI to establish daisy chain length
** - load patch
** - initialize general parameters
** - calibrate madc
** - bring up DC/DC converters
** - calibrate remaining items except madc & lb
**
** Input Parameters: 
** pProslic: pointer to PROSLIC object array
** fault: error code
**
** Return:
** error code
*/

int Si3226x_Init (proslicChanType_ptr *pProslic, int size){
    /*
    ** This function will initialize the chipRev and chipType members in pProslic
    ** as well as load the initialization structures.
    */
    uInt8 data;
    uInt8 calSetup[] = {0x00, 0x00, 0x01, 0x80};  /* CALR0-CALR3 */ 
    int k;
    const proslicPatch *patch = NULL;

    /*
    ** Identify channel type (ProSLIC or DAA) before initialization.
    ** Channels identified as DAA channels will not be modified during
    ** the ProSLIC initialization
    */
    for (k=0;k<size;k++)
    {
        pProslic[k]->channelType = identifyChannelType(pProslic[k]);
#ifdef ENABLE_DEBUG
        if(pProslic[k]->debugMode) 
        {
            if(pProslic[k]->channelType == PROSLIC)
                LOGPRINT("si3226x : Channel %d : Type = PROSLIC\n",pProslic[k]->channel);
            else if(pProslic[k]->channelType == DAA)
                LOGPRINT("si3226x : Channel %d : Type = DAA\n",pProslic[k]->channel);
            else
                LOGPRINT("si3226x : Channel %d : Type = UNKNOWN\n",pProslic[k]->channel);
        }
#endif
    }


    /*
    ** Read channel id to establish chipRev and chipType
    */
    for (k=0;k<size;k++)
    {
        if(pProslic[k]->channelType == PROSLIC)
        {
            data = pProslic[k]->ReadRegX(pProslic[k]->pProHWX,pProslic[k]->channel,ID);
            pProslic[k]->deviceId->chipRev = data&0x7;
            pProslic[k]->deviceId->chipType = getChipType(data);

#ifdef ENABLE_DEBUG
			if(pProslic[k]->debugMode)
			{
				LOGPRINT("si3226x : Channel %d : Chip Type %d\n",pProslic[k]->channel,pProslic[k]->deviceId->chipType);		
				LOGPRINT("si3226x : Channel %d : Chip Rev %d\n",pProslic[k]->channel,pProslic[k]->deviceId->chipRev);
			}
#endif
        }
    }

    /*
    ** Probe each channel and enable all channels that respond 
    */
    for (k=0;k<size;k++){
        if ((pProslic[k]->channelEnable)&&(pProslic[k]->channelType == PROSLIC)){
            pProslic[k]->WriteRegX(pProslic[k]->pProHWX,pProslic[k]->channel,PCMRXLO,0x5a);
            if (pProslic[k]->ReadRegX(pProslic[k]->pProHWX,pProslic[k]->channel,PCMRXLO) != 0x5A){
                pProslic[k]->channelEnable = 0;
                pProslic[k]->error = RC_SPI_FAIL;
                return pProslic[k]->error;    /* Halt init if SPI fail */
            }
        }
    }
        
    /**
     * Load patch (do not enable until patch loaded on all channels)
     */
    for (k=0;k<size;k++)
    {
        if ((pProslic[k]->channelEnable)&&(pProslic[k]->channelType == PROSLIC))
        {
            if (pProslic[k]->deviceId->chipRev == SI3226X_REVB ) 
			{
#ifdef SIVOICE_MULTI_BOM_SUPPORT
				if(Si3226x_General_Configuration.bomOpt == BO_DCDC_FLYBACK)
				{
					if(Si3226x_General_Configuration.batType == BO_DCDC_TRACKING)
					{
						patch = &(SI3226X_PATCH_B_FLBK);
					}
					else
					{
						patch = &(SI3226X_PATCH_B_FIXRL);
					}
				}
				else
				{
#ifdef ENABLE_DEBUG
					if(pProslic[k]->debugMode)
					{
						LOGPRINT("si3226x : Channel %d : Invalid Patch\n",pProslic[k]->channel);
					}
#endif
					pProslic[k]->channelEnable = 0;
					pProslic[k]->error = RC_INVALID_PATCH;
					return RC_INVALID_PATCH;
				}
#else
                patch = &(SI3226X_PATCH_B_DEFAULT);
#endif
			}
            else if (pProslic[k]->deviceId->chipRev == SI3226X_REVC ) 
			{
#ifdef SIVOICE_MULTI_BOM_SUPPORT
				if(Si3226x_General_Configuration.batType == BO_DCDC_TRACKING)
				{
					if(Si3226x_General_Configuration.bomOpt == BO_DCDC_FLYBACK)
					{
						patch = &(SI3226X_PATCH_C_FLBK);
					}
					else if(Si3226x_General_Configuration.bomOpt == BO_DCDC_QCUK)
					{
						patch = &(SI3226X_PATCH_C_QCUK);
					}
				}
				else if((Si3226x_General_Configuration.batType == BO_DCDC_TSS)&&(Si3226x_General_Configuration.bomOpt == BO_DCDC_FLYBACK))
				{
					patch = &(SI3226X_PATCH_C_TSS);
				}
                else if((Si3226x_General_Configuration.batType == BO_DCDC_TSS_ISO)&&(Si3226x_General_Configuration.bomOpt == BO_DCDC_FLYBACK))
				{
					patch = &(SI3226X_PATCH_C_TSS_ISO);
				}
				else
				{
#ifdef ENABLE_DEBUG
					if(pProslic[k]->debugMode)
					{
						LOGPRINT("si3226x : Channel %d : Invalid Patch\n",pProslic[k]->channel);
					}
#endif
					pProslic[k]->channelEnable = 0;
					pProslic[k]->error = RC_INVALID_PATCH;
					return RC_INVALID_PATCH;
				}
#else
                patch = &(SI3226X_PATCH_C_DEFAULT);
#endif
			}
            else
			{
#ifdef ENABLE_DEBUG
				if (pProslic[k]->debugMode)
				{
					LOGPRINT("si3226x : Channel %d : Unsupported Device Revision (%d)\n",pProslic[k]->channel,pProslic[k]->deviceId->chipRev );
				}
#endif
				pProslic[k]->channelEnable = 0;
				pProslic[k]->error = RC_UNSUPPORTED_DEVICE_REV;
                return RC_UNSUPPORTED_DEVICE_REV;
			}
                
            Si3226x_LoadPatch(pProslic[k],patch);
        }
    }

    /**
     * Verify and Enable Patch
     */
    for (k=0;k<size;k++)
    {
        if ((pProslic[k]->channelEnable)&&(pProslic[k]->channelType == PROSLIC))
        {
#ifdef DISABLE_VERIFY_PATCH
            Si3226x_EnablePatch(pProslic[k]);
#else
            data = Si3226x_VerifyPatch(pProslic[k],patch);
            if (data)
            {
                pProslic[k]->channelEnable=0;
                pProslic[k]->error = data;  
                return data;   /* Stop Init if patch load failure occurs */
            } 
            else 
            {
                Si3226x_EnablePatch(pProslic[k]);
            }
#endif                          

        }
    }
    
    /*
    ** Load general parameters - includes all BOM dependencies
    **
    ** First qualify general parameters by identifying valid device key.  This
    ** will prevent inadvertent use of other device's preset files, which could
    ** lead to improper initialization and high current states.
    */

    data = Si3226x_General_Configuration.device_key;

    if((data < DEVICE_KEY_MIN)||(data > DEVICE_KEY_MAX)) 
    {
        pProslic[0]->error = RC_INVALID_GEN_PARAM;
        return pProslic[0]->error;
    }
    
    for (k=0;k<size;k++){ 
        if ((pProslic[k]->channelEnable)&&(pProslic[k]->channelType == PROSLIC)){
            setUserMode(pProslic[k],TRUE);      
            /* Force pwrsave off and disable AUTO-tracking - set to user configured state after cal */
            pProslic[k]->WriteRegX(pProslic[k]->pProHWX, pProslic[k]->channel,ENHANCE,0);
            pProslic[k]->WriteRegX(pProslic[k]->pProHWX, pProslic[k]->channel,AUTO,0x2F); 

            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,BAT_HYST,Si3226x_General_Configuration.bat_hyst);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,VBATR_EXPECT,Si3226x_General_Configuration.vbatr_expect);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,VBATH_EXPECT,Si3226x_General_Configuration.vbath_expect);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,VBAT_TRACK_MIN,Si3226x_General_Configuration.vbat_track_min);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,VBAT_TRACK_MIN_RNG,Si3226x_General_Configuration.vbat_track_min_rng);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_VERR,Si3226x_General_Configuration.dcdc_verr);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_UVHYST,Si3226x_General_Configuration.dcdc_uvhyst);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_UVTHRESH,Si3226x_General_Configuration.dcdc_uvthresh);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_OVTHRESH,Si3226x_General_Configuration.dcdc_ovthresh);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_SWDRV_POL,Si3226x_General_Configuration.dcdc_swdrv_pol);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_UVPOL,Si3226x_General_Configuration.dcdc_uvpol);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_VREF_CTRL,Si3226x_General_Configuration.dcdc_vref_ctrl);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_RNGTYPE,Si3226x_General_Configuration.dcdc_rngtype);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_ANA_GAIN,Si3226x_General_Configuration.dcdc_ana_gain);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_ANA_TOFF,Si3226x_General_Configuration.dcdc_ana_toff);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_ANA_TONMIN,Si3226x_General_Configuration.dcdc_ana_tonmin);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_ANA_TONMAX,Si3226x_General_Configuration.dcdc_ana_tonmax);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_ANA_DSHIFT,Si3226x_General_Configuration.dcdc_ana_dshift);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_ANA_LPOLY,Si3226x_General_Configuration.dcdc_ana_lpoly);

            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,COEF_P_HVIC,Si3226x_General_Configuration.coef_p_hvic);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,P_TH_HVIC,Si3226x_General_Configuration.p_th_hvic);
            pProslic[k]->WriteRegX(pProslic[k]->pProHWX, pProslic[k]->channel,CM_CLAMP,Si3226x_General_Configuration.cm_clamp);
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,SCALE_KAUDIO,Si3226x_General_Configuration.scale_kaudio);


            /* Hardcoded mods to default settings */
            data = pProslic[k]->ReadRegX(pProslic[k]->pProHWX, pProslic[k]->channel,GPIO_CFG1);
            data &= 0xF9;  /* Clear DIR for GPIO 1&2 */
            data |= 0x60;  /* Set ANA mode for GPIO 1&2 */
            pProslic[k]->WriteRegX(pProslic[k]->pProHWX, pProslic[k]->channel,GPIO_CFG1,data); /* coarse sensors analog mode */
            pProslic[k]->WriteRegX(pProslic[k]->pProHWX, pProslic[k]->channel,PDN,0x80); /* madc powered in open state */
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,PWRSAVE_TIMER,0xFFF0000L); /* 4sec pwrsave timer*/
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,TXACHPF_A1_1,0x71EB851L); /* Fix HPF corner */
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,ROW0_C2, 0x723F235L);   /* improved DTMF det */
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,ROW1_C2, 0x57A9804L);   /* improved DTMF det */
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_UV_DEBOUNCE, 0x100000L); 
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_OV_DEBOUNCE, 0x0L); 
            pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,XTALK_TIMER,0x36000L); /* xtalk fix */
			if(pProslic[k]->deviceId->chipRev == SI3226X_REVB)
			{
				pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_OVTHRESH,0x1700000L);
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,LKG_OFHK_OFFSET_REVB,Si3226x_General_Configuration.lkg_ofhk_offset);   
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,LKG_LB_OFFSET_REVB,Si3226x_General_Configuration.lkg_lb_offset); 
			}
			else
			{
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,LKG_OFHK_OFFSET,Si3226x_General_Configuration.lkg_ofhk_offset);   
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,LKG_LB_OFFSET,Si3226x_General_Configuration.lkg_lb_offset); 
			}

            /* Additional modifications if using fixed-rail or external battery */
            if(Si3226x_General_Configuration.batType != BO_DCDC_TRACKING)
            {
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_VERR_HYST,Si3226x_General_Configuration.dcdc_verr_hyst);
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,PD_OVLO,Si3226x_General_Configuration.pd_ovlo);
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,PD_UVLO,Si3226x_General_Configuration.pd_uvlo);
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,PD_OCLO,Si3226x_General_Configuration.pd_oclo);

                if((Si3226x_General_Configuration.batType == BO_DCDC_TSS)||(Si3226x_General_Configuration.batType == BO_DCDC_TSS_ISO))
                {
                    pProslic[k]->WriteRegX(pProslic[k]->pProHWX, pProslic[k]->channel,OFFLOAD,0x3); /* Enable offload and vbat_l */
                }
                else
                {
                    pProslic[k]->WriteRegX(pProslic[k]->pProHWX, pProslic[k]->channel,OFFLOAD,0x13); /* Enable offload and vbat_l, disable fixed rail battery management. */
                }

                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,VBATL_EXPECT, VBATL_13V); /* force vbatl 13v to keep cm recalc */                              
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,MADC_VDC_SCALE, FIXRL_VDC_SCALE); 
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_UV_DEBOUNCE, 0L); 
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_OV_DEBOUNCE, 0xD00000L); 
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_OIMASK, 0xA00000L); 
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,DCDC_PD_ANA, 0x300000);    
			    if(pProslic[k]->deviceId->chipRev == SI3226X_REVB)
			    {
					pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,BAT_HYST,0xAC480L); 
                    pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,VBATH_DELTA_REVB,0x1000000L); 
                    pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,UVTHRESH_MAX_REVB,Si3226x_General_Configuration.uvthresh_max); 
                    pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,UVTHRESH_SCALE_REVB,Si3226x_General_Configuration.uvthresh_scale); 
                    pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,UVTHRESH_BIAS_REVB,Si3226x_General_Configuration.uvthresh_bias); 
				}
				else
				{
                    pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,VCM_HYST,0x206280L); /* 2v */
                    pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,VBATH_DELTA,Si3226x_General_Configuration.vbath_delta); 
                    pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,UVTHRESH_MAX,Si3226x_General_Configuration.uvthresh_max); 
                    pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,UVTHRESH_SCALE,Si3226x_General_Configuration.uvthresh_scale); 
                    pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,UVTHRESH_BIAS,Si3226x_General_Configuration.uvthresh_bias); 
				}
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,P_TH_OFFLOAD, 0x280CBFL); /* 1.1W @ 60C */ 
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,PD_OFFLD_DAC,0x200000L);
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,PD_OFFLD_GM,0x200000L);
           }

            setUserMode(pProslic[k],FALSE);
        }
    }

    /*
    ** Calibrate (madc offset)
    */
    Si3226x_Calibrate(pProslic,size,calSetup,TIMEOUT_MADC_CAL);



    /*
    ** Bring up DC/DC converters sequentially to minimize
    ** peak power demand on VDC
    */
    for (k=0;k<size;k++)
    { 
        if ((pProslic[k]->channelEnable)&&(pProslic[k]->channelType == PROSLIC))
        {
            setUserMode(pProslic[k],TRUE);      
            pProslic[k]->error = Si3226x_PowerUpConverter(pProslic[k]);
            setUserMode(pProslic[k],FALSE);
        }
    }
  

    /*
    ** Calibrate remaining cals (except madc, lb)
    */
    calSetup[1] = SI3226X_CAL_STD_CALR1;
    calSetup[2] = SI3226X_CAL_STD_CALR2;
    Si3226x_Calibrate(pProslic,size,calSetup,TIMEOUT_GEN_CAL);

    /*
    ** Apply user configured ENHANCE and AUTO 
    */
    for (k=0;k<size;k++){ 
        if ((pProslic[k]->channelEnable)&&(pProslic[k]->channelType == PROSLIC)){
            setUserMode(pProslic[k],TRUE);      
            pProslic[k]->WriteRegX(pProslic[k]->pProHWX, pProslic[k]->channel,ENHANCE,Si3226x_General_Configuration.enhance);
            pProslic[k]->WriteRegX(pProslic[k]->pProHWX, pProslic[k]->channel,AUTO,Si3226x_General_Configuration.autoRegister); 
            pProslic[k]->WriteRegX(pProslic[k]->pProHWX, pProslic[k]->channel,ZCAL_EN,Si3226x_General_Configuration.zcal_en); 
            /* Rev A Workarounds - done after cal */
            if (pProslic[k]->deviceId->chipRev == SI3226X_REVA ) { 
                pProslic[k]->WriteRAMX(pProslic[k]->pProHWX, pProslic[k]->channel,PD_HVIC,0x200000L); /* HVIC timing issue */
            }

            setUserMode(pProslic[k],FALSE);  
        }
    }

    
    return 0;
}




/*
** Function: Si3226x_PrintDebugData
**
** Description: 
** Register and RAM dump utility
**
** Input Parameters: 
** pProslic: pointer to PROSLIC channel object
** broadcast:  broadcast flag
**
** Return:
** 0
*/
int Si3226x_PrintDebugData (proslicChanType *pProslic){
#ifdef ENABLE_DEBUG
    int i;
    for (i=0;i<99;i++)
        LOGPRINT ("Si3226x Register %d = %X\n",i,ReadReg(pProHW,pProslic->channel,i));
    for (i=0;i<1024;i++)
        LOGPRINT ("Si3226x RAM %d = %X\n",i,(unsigned int)(ReadRAM(pProHW,pProslic->channel,i)));
#endif
    return 0;
}



/*
** Function: Si3226x_LBCal
**
** Description: 
** Run canned longitudinal balance calibration.  Each channel
** may be calibrated in parallel since there are no shared
** resources between si3226x devices.
**
** Input Parameters: 
** pProslic: pointer to array of PROSLIC channel objects
** size:     number of PROSLIC channel objects   
**
** Return:
** 0
*/
int Si3226x_LBCal(proslicChanType_ptr *pProslic, int size)
{
    int k;
    int i;
    uInt8 data;
    int timeout = 0;

#ifdef DISABLE_MALLOC
    uInt8 lf[64]; 

    if (size > 64) {
        LOGPRINT("Too many channels - wanted %d, max of %d\n",
                 size, 64);
        return RC_NO_MEM;
    }
#else
    uInt8 *lf;

    lf = malloc(size * sizeof(uInt8));
    if (lf == 0) {
        return RC_NO_MEM;
    }
#endif


    /* Start Cal on each channel first */
    for (k=0;k<size;k++)
    {
        if (pProslic[k]->channelEnable)
        {
#ifdef ENABLE_DEBUG
            if(pProslic[k]->debugMode)
            {
                LOGPRINT("Starting LB Cal on channel %d\n",
                         pProslic[k]->channel);
            }
#endif
            lf[k] = pProslic[k]->ReadRegX(pProslic[k]->pProHWX, 
                                          pProslic[k]->channel,
                                          LINEFEED); 
            Si3226x_SetLinefeedStatus(pProslic[k],
                                      LF_FWD_ACTIVE);
                        
            pProslic[k]->WriteRegX(pProslic[k]->pProHWX, 
                                   pProslic[k]->channel,
                                   CALR0,
                                   CAL_LB_ALL); /* enable LB cal */

            pProslic[k]->WriteRegX(pProslic[k]->pProHWX, 
                                   pProslic[k]->channel,
                                   CALR3,
                                   0x80); /* start cal */


            i=0;
            do {
                data = pProslic[k]->ReadRegX(pProslic[k]->pProHWX,
                                             pProslic[k]->channel,
                                             CALR3);
                pProslic[k]->DelayX(pProslic[k]->pProTimerX, 10);

            } while (data&0x80 && ++i<=TIMEOUT_LB_CAL);

            if (i >= TIMEOUT_LB_CAL) {
#ifdef ENABLE_DEBUG
                if (pProslic[k]->debugMode)
                    LOGPRINT("Calibration timeout channel %d\n",
                             pProslic[k]->channel);
#endif
                pProslic[k]->error = RC_CAL_TIMEOUT;
                pProslic[k]->WriteRegX(pProslic[k]->pProHWX, 
                                       pProslic[k]->channel,
                                       LINEFEED,
                                       LF_OPEN); 
                timeout = 1;
            } else {

           
                pProslic[k]->WriteRegX(pProslic[k]->pProHWX, 
                                       pProslic[k]->channel,
                                       LINEFEED,
                                       lf[k]); 
            }
        }
    }

#ifndef DISABLE_MALLOC
    free(lf);
#endif
    if (timeout != 0) {
        return RC_CAL_TIMEOUT;
    } else {
        return 0;
    }
}



/*
** Function: Si3226x_GetLBCalResult
**
** Description: 
** Read applicable calibration coefficients
**
** Input Parameters: 
** pProslic: pointer to PROSLIC channel object
** resultx:  pointer to 4 RAM results
**
** Return:
** 0
*/
int Si3226x_GetLBCalResult (proslicChanType *pProslic,int32 *result1,int32 *result2,int32 *result3,int32 *result4){
    setUserMode(pProslic,TRUE);
    *result1 = ReadRAM(pProHW,pProslic->channel,CMDAC_FWD);
    *result2 = ReadRAM(pProHW,pProslic->channel,CMDAC_REV);
    *result3 = ReadRAM(pProHW,pProslic->channel,CAL_TRNRD_DACT);
    *result4 = ReadRAM(pProHW,pProslic->channel,CAL_TRNRD_DACR);
    setUserMode(pProslic,FALSE);
    return 0;
}

/*
** Function: Si3226x_GetLBCalResultPacked
**
** Description: 
** Read applicable calibration coefficients
** and pack into single 32bit word
**
** Input Parameters: 
** pProslic: pointer to PROSLIC channel object
** result:   pointer to packed result
**
** Return:
** 0
**
** Packed Result Format
**
** Bits 31:24   CMDAC_FWD[
*/
int Si3226x_GetLBCalResultPacked (proslicChanType *pProslic,int32 *result){
    int32 tmpResult;
    setUserMode(pProslic,TRUE);
    tmpResult = ReadRAM(pProHW,pProslic->channel,CMDAC_FWD);
    *result = (tmpResult<<6)&0xff000000L;
    tmpResult = ReadRAM(pProHW,pProslic->channel,CMDAC_REV);
    *result |= (tmpResult>>1)&0x00ff0000L;
    tmpResult = ReadRAM(pProHW,pProslic->channel,CAL_TRNRD_DACT);
    *result |= (tmpResult>>5)&0x0000ff00L;
    tmpResult = ReadRAM(pProHW,pProslic->channel,CAL_TRNRD_DACR);
    *result |= (tmpResult>>13)&0x000000ffL;
    setUserMode(pProslic,FALSE);
    return 0;
}
/*
** Function: Si3226x_LoadPreviousLBCal
**
** Description: 
** Load applicable calibration coefficients
**
** Input Parameters: 
** pProslic: pointer to PROSLIC channel object
** resultx:  pointer to 4 RAM results
**
** Return:
** 0
*/
int Si3226x_LoadPreviousLBCal (proslicChanType *pProslic,int32 result1,int32 result2,int32 result3,int32 result4){
    setUserMode(pProslic,TRUE);
    WriteRAM(pProHW,pProslic->channel,CMDAC_FWD,result1);
    WriteRAM(pProHW,pProslic->channel,CMDAC_REV,result2);
    WriteRAM(pProHW,pProslic->channel,CAL_TRNRD_DACT,result3);
    WriteRAM(pProHW,pProslic->channel,CAL_TRNRD_DACR,result4);
    setUserMode(pProslic,FALSE);
    return 0;
}


/*
** Function: Si3226x_LoadPreviousLBCalPacked
**
** Description: 
** Load applicable calibration coefficients
**
** Input Parameters: 
** pProslic: pointer to PROSLIC channel object
** result:   pointer to packed cal results
**
** Return:
** 0
*/
int Si3226x_LoadPreviousLBCalPacked (proslicChanType *pProslic,int32 *result){
    int32 ramVal;
    setUserMode(pProslic,TRUE);
    ramVal = (*result&0xff000000L)>>6;
    WriteRAM(pProHW,pProslic->channel,CMDAC_FWD,ramVal);
    ramVal = (*result&0x00ff0000L)<<1;
    WriteRAM(pProHW,pProslic->channel,CMDAC_REV,ramVal);
    ramVal = (*result&0x0000ff00L)<<5;
    WriteRAM(pProHW,pProslic->channel,CAL_TRNRD_DACT,ramVal);
    ramVal = (*result&0x000000ffL)<<13;
    WriteRAM(pProHW,pProslic->channel,CAL_TRNRD_DACR,ramVal);
#ifdef API_TEST
    ramVal = ReadRAM(pProHW,pProslic->channel,CMDAC_FWD);
    LOGPRINT ("UNPACKED CMDAC_FWD = %08x\n",ramVal);
    ramVal = ReadRAM(pProHW,pProslic->channel,CMDAC_REV);
    LOGPRINT ("UNPACKED CMDAC_REF = %08x\n",ramVal);
    ramVal = ReadRAM(pProHW,pProslic->channel,CAL_TRNRD_DACT);
    LOGPRINT ("UNPACKED CAL_TRNRD_DACT = %08x\n",ramVal);
    ramVal = ReadRAM(pProHW,pProslic->channel,CAL_TRNRD_DACR);
    LOGPRINT ("UNPACKED CAL_TRNRD_DACR = %08x\n",ramVal);
#endif
    setUserMode(pProslic,FALSE);
    return 0;
}

/*
** Function: Si3226x_LoadRegTables
**
** Description: 
** Generic register and ram table loader
**
** Input Parameters:
** pProslic:  pointer to PROSLIC channel object
** pRamTable: pointer to PROSLIC ram table
** pRegTable: pointer to PROSLIC reg table
** size:      number of channels
**
** Return:
** 0
*/
int Si3226x_LoadRegTables (proslicChanType_ptr *pProslic, ProslicRAMInit *pRamTable, ProslicRegInit *pRegTable, int size){
    uInt16 i;
    for (i=0;i<size;i++){
        if (pProslic[i]->channelEnable)
            LoadRegTables(pProslic[i],pRamTable,pRegTable,0);
    }
    return 0;
}


/*
** Function: Si3226x_LoadPatch
**
** Description: 
** Calls patch loading function
**
** Input Parameters:
** pProslic:   pointer to PROSLIC channel obj
** pPatch:     pointer to PROSLIC patch obj
**
** Returns:
** 0
*/
int Si3226x_LoadPatch (proslicChanType *pProslic, const proslicPatch *pPatch){ 
    LoadSi3226xPatch(pProslic,pPatch,0);
    return 0;
}


/*
** Function: Si3226x_VerifyPatch
**
** Description: 
** Veriy patch load
**
** Input Parameters:
** pProslic:   pointer to PROSLIC channel obj
** pPatch:     pointer to PROSLIC patch obj
**
** Returns:
** 0
*/
int Si3226x_VerifyPatch (proslicChanType *pProslic, const proslicPatch *pPatch){ 
int loop;
uInt8 jmp_table_low;
uInt16 jmp_table_high;
uInt8 data; 
uInt32 ramdata;
int err_low = 0;
int err_high = 0;
int err_ram = 0;

    if (pPatch == NULL)
        return RC_NONE;

    setUserMode (pProslic,TRUE); /*make sure we are in user mode to read patch*/

    WriteReg (pProHW, pProslic->channel, JMPEN,0); /*disable the patch*/

    WriteRAM(pProHW, pProslic->channel,PRAM_ADDR, 0); /*write patch ram address register*/
        
    /* If the data is all 0, you have hit the end of the programmed values and can stop loading.*/
    for (loop=0; loop<PATCH_MAX_SIZE; loop++){
        if (pPatch->patchData[loop] != 0){
            ramdata = ReadRAM(pProHW, pProslic->channel,PRAM_DATA); /*note. data is shifted*/
            if (pPatch->patchData[loop]<<9 != ramdata){
#ifdef ENABLE_DEBUG
                if(pProslic->debugMode)
                {
                    LOGPRINT("ERROR : Addr: %d   Expected: %d   Actual: %d\n", loop, (int)(pPatch->patchData[loop]<<9), (int)(ramdata));
                }
#endif
                loop = PATCH_MAX_SIZE;                  
                err_ram = 1;
            }
        }
        else
            loop = PATCH_MAX_SIZE;
    }

    if (err_ram){
#ifdef ENABLE_DEBUG
        if (pProslic->debugMode)
            LOGPRINT("ERROR :  patch verify RAM : channel %d\n",pProslic->channel);
#endif
    }
        
    /*zero out RAM_ADDR_HI*/
    WriteReg (pProHW, pProslic->channel, RAM_ADDR_HI,0);

    /**
     * Verify jump table low entries
     */
    jmp_table_low=PATCH_JMPTBL_LOW_ADDR;
    for (loop=0;loop<PATCH_NUM_LOW_ENTRIES;loop++){
        /* check the jump table with the new values.*/
        if (pPatch->patchEntries[loop] != 0){
            data = ReadReg (pProHW, pProslic->channel, jmp_table_low);
            if (data != ((pPatch->patchEntries[loop])&0xff))
                err_low = 1;
            data = ReadReg (pProHW, pProslic->channel, jmp_table_low+1);
            if (data != (pPatch->patchEntries[loop]>>8))
                err_low = 1;
        }
        jmp_table_low+=2;
    }
    if (err_low){
#ifdef ENABLE_DEBUG
        if (pProslic->debugMode)
            LOGPRINT("ERROR :  patch verify table low : channel %d\n",pProslic->channel);
#endif
    }

    /**
     * Verify jump table high entries
     */
    jmp_table_high=PATCH_JMPTBL_HIGH_ADDR;
    for (loop=0;loop<PATCH_NUM_HIGH_ENTRIES;loop++)
    {
        if (pPatch->patchEntries[loop+PATCH_NUM_LOW_ENTRIES] != 0)
        {
            ramdata = ReadRAM (pProHW, pProslic->channel, jmp_table_high);
            if (ramdata != (((uInt32)(pPatch->patchEntries[loop+PATCH_NUM_LOW_ENTRIES]))&(0x00001fffL)))
                err_high = 1;
        }
        jmp_table_high++;
    }
    if (err_high){ 
#ifdef ENABLE_DEBUG
        if (pProslic->debugMode)
            LOGPRINT("ERROR :  patch verify table high : channel %d\n",pProslic->channel);
#endif
    }


    /**
     * If no errors, re-enable the patch
     */

    if(!(err_ram | err_low | err_high))
        WriteReg (pProHW, pProslic->channel, JMPEN,1); /*enable the patch*/
    
    setUserMode(pProslic,FALSE); /*turn off user mode*/

    if(err_ram)
        return RC_PATCH_RAM_VERIFY_FAIL;
    else if(err_low | err_high)
        return RC_PATCH_ENTRY_VERIFY_FAIL;
    else
        return RC_NONE;
}


/*
** Function: Si3226x_SetLoopbackMode
**
** Description: 
** Program desired loopback test mode
**
** Input Parameters:
** pProslic:   pointer to PROSLIC channel obj
** newMode:    desired loopback mode tag
**
** Returns:
** 0
*/
int Si3226x_SetLoopbackMode (proslicChanType_ptr pProslic, ProslicLoopbackModes newMode){
    uInt8 regTemp;
    regTemp = ReadReg (pProHW,pProslic->channel,LOOPBACK);
    switch (newMode){
    case PROSLIC_LOOPBACK_NONE:
        WriteReg (pProHW,pProslic->channel,LOOPBACK,regTemp&~(0x11));
        break;
    case PROSLIC_LOOPBACK_DIG:
        WriteReg (pProHW,pProslic->channel,LOOPBACK,regTemp|(0x1));
        break;
    case PROSLIC_LOOPBACK_ANA:
        WriteReg (pProHW,pProslic->channel,LOOPBACK,regTemp|(0x10));
        break;
    }
    return 0;
}

/*
** Function: Si3226x_SetMuteStatus
**
** Description: 
** configure RX and TX path mutes
**
** Input Parameters:
** pProslic:   pointer to PROSLIC channel obj
** muteEn:     mute configuration tag
**
** Returns:
** 0
*/
int Si3226x_SetMuteStatus (proslicChanType_ptr pProslic, ProslicMuteModes muteEn){
    uInt8 regTemp;
    uInt8 newRegValue;

    regTemp = ReadReg (pProHW,pProslic->channel,DIGCON);
    newRegValue = regTemp &~(0x3);

    WriteReg (pProHW,pProslic->channel,DIGCON,regTemp&~(0x3));
       
    if (muteEn & PROSLIC_MUTE_RX){
        newRegValue |= 1;    
    }
    if (muteEn & PROSLIC_MUTE_TX){
        newRegValue |= 2;    
    }

    if(newRegValue != regTemp)
    {
        WriteReg (pProHW,pProslic->channel,DIGCON,newRegValue);
    }
    return 0;
}

/*
** Function: Si3226x_EnableInterrupts
**
** Description: 
** Enables interrupts
**
** Input Parameters:
** pProslic:   pointer to PROSLIC channel obj
**
** Returns:
** 0
*/
int Si3226x_EnableInterrupts (proslicChanType_ptr pProslic){
    WriteReg (pProHW,pProslic->channel,IRQEN1,Si3226x_General_Configuration.irqen1);
    WriteReg (pProHW,pProslic->channel,IRQEN2,Si3226x_General_Configuration.irqen2);
    WriteReg (pProHW,pProslic->channel,IRQEN3,Si3226x_General_Configuration.irqen3);
    WriteReg (pProHW,pProslic->channel,IRQEN4,Si3226x_General_Configuration.irqen4);
    return 0;
}

/*
** Function: Si3226x_DisableInterrupts
**
** Description: 
** Disables/clears interrupts
**
** Input Parameters:
** pProslic:   pointer to PROSLIC channel obj
**
** Returns:
** 0
*/
int Si3226x_DisableInterrupts (proslicChanType_ptr pProslic){

    uInt8 data[4];
    WriteReg (pProHW,pProslic->channel,IRQEN1,0);
    WriteReg (pProHW,pProslic->channel,IRQEN2,0);
    WriteReg (pProHW,pProslic->channel,IRQEN3,0);
    WriteReg (pProHW,pProslic->channel,IRQEN4,0);

            
    data[0] = ReadReg(pProHW,pProslic->channel,IRQ1);
    data[1] = ReadReg(pProHW,pProslic->channel,IRQ2);
    data[2] = ReadReg(pProHW,pProslic->channel,IRQ3);
    data[3] = ReadReg(pProHW,pProslic->channel,IRQ4);
#ifdef GCI_MODE
    WriteReg(pProHW,pProslic->channel,IRQ1,data[0]); /*clear interrupts (gci only)*/
    WriteReg(pProHW,pProslic->channel,IRQ2,data[1]);
    WriteReg(pProHW,pProslic->channel,IRQ3,data[2]);
    WriteReg(pProHW,pProslic->channel,IRQ4,data[3]);
#endif
    return RC_NONE;
}

/*
** Function: Si3226x_CheckCIDBuffer
**
** Description: 
** configure fsk
**
** Input Parameters:
** pProslic:        pointer to PROSLIC channel obj
** fskBufAvail:     fsk buffer available flag
**
** Returns:
** 0
*/
int Si3226x_CheckCIDBuffer (proslicChanType *pProslic, uInt8 *fskBufAvail){
    uInt8 data;
    data = ReadReg(pProHW,pProslic->channel,IRQ1);
    WriteReg(pProHW,pProslic->channel,IRQ1,data); /*clear (for GCI)*/
    *fskBufAvail = (data&0x40) ? 1 : 0;
    return 0;
}


/*
**
** PROSLIC CONFIGURATION FUNCTIONS
**
*/

/*
** Function: Si3226x_RingSetup
**
** Description: 
** configure ringing
**
** Input Parameters:
** pProslic:   pointer to PROSLIC channel obj
** preset:     ring preset
**
** Returns:
** 0
*/
#ifndef DISABLE_RING_SETUP
int Si3226x_RingSetup (proslicChanType *pProslic, int preset){

    WriteRAM(pProHW,pProslic->channel,RTPER,Si3226x_Ring_Presets[preset].rtper);
    WriteRAM(pProHW,pProslic->channel,RINGFR,Si3226x_Ring_Presets[preset].freq);
    WriteRAM(pProHW,pProslic->channel,RINGAMP,Si3226x_Ring_Presets[preset].amp);
    WriteRAM(pProHW,pProslic->channel,RINGPHAS,Si3226x_Ring_Presets[preset].phas);
    WriteRAM(pProHW,pProslic->channel,RINGOF,Si3226x_Ring_Presets[preset].offset);
    WriteRAM(pProHW,pProslic->channel,SLOPE_RING,Si3226x_Ring_Presets[preset].slope_ring);
    WriteRAM(pProHW,pProslic->channel,IRING_LIM,Si3226x_Ring_Presets[preset].iring_lim);
    WriteRAM(pProHW,pProslic->channel,RTACTH,Si3226x_Ring_Presets[preset].rtacth);
    WriteRAM(pProHW,pProslic->channel,RTDCTH,Si3226x_Ring_Presets[preset].rtdcth);
    WriteRAM(pProHW,pProslic->channel,RTACDB,Si3226x_Ring_Presets[preset].rtacdb);
    WriteRAM(pProHW,pProslic->channel,RTDCDB,Si3226x_Ring_Presets[preset].rtdcdb);
    WriteRAM(pProHW,pProslic->channel,VOV_RING_BAT,Si3226x_Ring_Presets[preset].vov_ring_bat);
    WriteRAM(pProHW,pProslic->channel,VOV_RING_GND,Si3226x_Ring_Presets[preset].vov_ring_gnd);
    /* Always limit VBATR_EXPECT to the general configuration maximum */
    if(Si3226x_Ring_Presets[preset].vbatr_expect > Si3226x_General_Configuration.vbatr_expect)
    {
        WriteRAM(pProHW,pProslic->channel,VBATR_EXPECT,Si3226x_General_Configuration.vbatr_expect);
#ifdef ENABLE_DEBUG
	if(pProslic->debugMode)
	{
	    LOGPRINT("ProSLIC_RingSetup : VBATR_EXPECT : Clamped to Gen Conf Limit\n");
	}
#endif
    }
    else
    {
        WriteRAM(pProHW,pProslic->channel,VBATR_EXPECT,Si3226x_Ring_Presets[preset].vbatr_expect);
    }
    WriteReg(pProHW,pProslic->channel,RINGTALO,Si3226x_Ring_Presets[preset].talo);
    WriteReg(pProHW,pProslic->channel,RINGTAHI,Si3226x_Ring_Presets[preset].tahi);
    WriteReg(pProHW,pProslic->channel,RINGTILO,Si3226x_Ring_Presets[preset].tilo);
    WriteReg(pProHW,pProslic->channel,RINGTIHI,Si3226x_Ring_Presets[preset].tihi);
  
    WriteRAM(pProHW,pProslic->channel,DCDC_VREF_MIN_RNG,Si3226x_Ring_Presets[preset].vbat_track_min_rng);
    WriteReg(pProHW,pProslic->channel,RINGCON,Si3226x_Ring_Presets[preset].ringcon);
    WriteReg(pProHW,pProslic->channel,USERSTAT,Si3226x_Ring_Presets[preset].userstat);
    WriteRAM(pProHW,pProslic->channel,VCM_RING,Si3226x_Ring_Presets[preset].vcm_ring);
    WriteRAM(pProHW,pProslic->channel,VCM_RING_FIXED,Si3226x_Ring_Presets[preset].vcm_ring_fixed);
    WriteRAM(pProHW,pProslic->channel,DELTA_VCM,Si3226x_Ring_Presets[preset].delta_vcm);
    WriteRAM(pProHW,pProslic->channel,VOV_DCDC_SLOPE,Si3226x_Ring_Presets[preset].vov_dcdc_slope);
    WriteRAM(pProHW,pProslic->channel,VOV_DCDC_OS,Si3226x_Ring_Presets[preset].vov_dcdc_os);
    WriteRAM(pProHW,pProslic->channel,VOV_RING_BAT_MAX,Si3226x_Ring_Presets[preset].vov_ring_bat_max);

    setUserMode(pProslic,TRUE);
    WriteRAM(pProHW,pProslic->channel,DCDC_RNGTYPE,Si3226x_Ring_Presets[preset].dcdc_rngtype);
    setUserMode(pProslic,FALSE);

    return 0;
}
#endif
/*
** Function: Si3226x_ToneGenSetup
**
** Description: 
** configure tone generators
**
** Input Parameters:
** pProslic:   pointer to PROSLIC channel obj
** preset:     tone generator preset
**
** Returns:
** 0
*/
#ifndef DISABLE_TONE_SETUP
int Si3226x_ToneGenSetup (proslicChanType *pProslic, int preset){
    WriteRAM(pProHW,pProslic->channel,OSC1FREQ,Si3226x_Tone_Presets[preset].osc1.freq);
    WriteRAM(pProHW,pProslic->channel,OSC1AMP,Si3226x_Tone_Presets[preset].osc1.amp);
    WriteRAM(pProHW,pProslic->channel,OSC1PHAS,Si3226x_Tone_Presets[preset].osc1.phas);
    WriteReg(pProHW,pProslic->channel,O1TAHI,(Si3226x_Tone_Presets[preset].osc1.tahi));
    WriteReg(pProHW,pProslic->channel,O1TALO,(Si3226x_Tone_Presets[preset].osc1.talo));
    WriteReg(pProHW,pProslic->channel,O1TIHI,(Si3226x_Tone_Presets[preset].osc1.tihi));
    WriteReg(pProHW,pProslic->channel,O1TILO,(Si3226x_Tone_Presets[preset].osc1.tilo));
    WriteRAM(pProHW,pProslic->channel,OSC2FREQ,Si3226x_Tone_Presets[preset].osc2.freq);
    WriteRAM(pProHW,pProslic->channel,OSC2AMP,Si3226x_Tone_Presets[preset].osc2.amp);
    WriteRAM(pProHW,pProslic->channel,OSC2PHAS,Si3226x_Tone_Presets[preset].osc2.phas);
    WriteReg(pProHW,pProslic->channel,O2TAHI,(Si3226x_Tone_Presets[preset].osc2.tahi));
    WriteReg(pProHW,pProslic->channel,O2TALO,(Si3226x_Tone_Presets[preset].osc2.talo));
    WriteReg(pProHW,pProslic->channel,O2TIHI,(Si3226x_Tone_Presets[preset].osc2.tihi));
    WriteReg(pProHW,pProslic->channel,O2TILO,(Si3226x_Tone_Presets[preset].osc2.tilo));
    WriteReg(pProHW,pProslic->channel,OMODE,(Si3226x_Tone_Presets[preset].omode));
    return 0;
}
#endif
/*
** Function: Si3226x_FSKSetup
**
** Description: 
** configure fsk
**
** Input Parameters:
** pProslic:   pointer to PROSLIC channel obj
** preset:     fsk preset
**
** Returns:
** 0
*/
#ifndef DISABLE_FSK_SETUP
int Si3226x_FSKSetup (proslicChanType *pProslic, int preset){
    uInt8 data; 


    WriteReg(pProHW,pProslic->channel,O1TAHI,0);
    WriteReg(pProHW,pProslic->channel,O1TIHI,0);
    WriteReg(pProHW,pProslic->channel,O1TILO,0);
    WriteReg(pProHW,pProslic->channel,O1TALO,0x13);

    data = ReadReg(pProHW,pProslic->channel,OMODE);
    if (Si3226x_FSK_Presets[preset].eightBit)
        data |= 0x80;
    else 
        data &= ~(0x80);
    WriteReg(pProHW,pProslic->channel,FSKDEPTH,Si3226x_FSK_Presets[preset].fskdepth);
    WriteReg(pProHW,pProslic->channel,OMODE,data);
    WriteRAM(pProHW,pProslic->channel,FSK01,Si3226x_FSK_Presets[preset].fsk01);
    WriteRAM(pProHW,pProslic->channel,FSK10,Si3226x_FSK_Presets[preset].fsk10);
    WriteRAM(pProHW,pProslic->channel,FSKAMP0,Si3226x_FSK_Presets[preset].fskamp0);
    WriteRAM(pProHW,pProslic->channel,FSKAMP1,Si3226x_FSK_Presets[preset].fskamp1);
    WriteRAM(pProHW,pProslic->channel,FSKFREQ0,Si3226x_FSK_Presets[preset].fskfreq0);
    WriteRAM(pProHW,pProslic->channel,FSKFREQ1,Si3226x_FSK_Presets[preset].fskfreq1);
    return 0;
}
#endif

/*
 * Function: Si3226x_ModifyStartBits
 * 
 * Description: To change the FSK start/stop bits field.
 * Returns RC_NONE if OK.
 */
int Si3226x_ModifyCIDStartBits(proslicChanType_ptr pProslic, uInt8 enable_startStop)
{
	uInt8 data;

	if(pProslic->channelType != PROSLIC) 
	{
        return RC_CHANNEL_TYPE_ERR;
    }

	data = ReadReg(pProHW,pProslic->channel,OMODE);
	
	if(enable_startStop == FALSE)
	{
		data &= ~0x80;
	}
	else
	{
		data |= 0x80;
	}

	WriteReg(pProHW,pProslic->channel,OMODE,data);

	return RC_NONE;
}

/*
** Function: Si3226x_DTMFDecodeSetup
**
** Description: 
** configure dtmf decode
**
** Input Parameters:
** pProslic:   pointer to PROSLIC channel obj
** preset:     dtmf preset
**
** Returns:
** 0
*/
#ifndef DISABLE_DTMF_SETUP
int Si3226x_DTMFDecodeSetup (proslicChanType *pProslic, int preset){
        
    WriteRAM(pProHW,pProslic->channel,DTMFDTF_B0_1,Si3226x_DTMFDec_Presets[preset].dtmfdtf_b0_1);
    WriteRAM(pProHW,pProslic->channel,DTMFDTF_B1_1,Si3226x_DTMFDec_Presets[preset].dtmfdtf_b1_1);
    WriteRAM(pProHW,pProslic->channel,DTMFDTF_B2_1,Si3226x_DTMFDec_Presets[preset].dtmfdtf_b2_1);
    WriteRAM(pProHW,pProslic->channel,DTMFDTF_A1_1,Si3226x_DTMFDec_Presets[preset].dtmfdtf_a1_1);
    WriteRAM(pProHW,pProslic->channel,DTMFDTF_A2_1,Si3226x_DTMFDec_Presets[preset].dtmfdtf_a2_1);
    WriteRAM(pProHW,pProslic->channel,DTMFDTF_B0_2,Si3226x_DTMFDec_Presets[preset].dtmfdtf_b0_2);
    WriteRAM(pProHW,pProslic->channel,DTMFDTF_B1_2,Si3226x_DTMFDec_Presets[preset].dtmfdtf_b1_2);
    WriteRAM(pProHW,pProslic->channel,DTMFDTF_B2_2,Si3226x_DTMFDec_Presets[preset].dtmfdtf_b2_2);
    WriteRAM(pProHW,pProslic->channel,DTMFDTF_A1_2,Si3226x_DTMFDec_Presets[preset].dtmfdtf_a1_2);
    WriteRAM(pProHW,pProslic->channel,DTMFDTF_A2_2,Si3226x_DTMFDec_Presets[preset].dtmfdtf_a2_2);
    WriteRAM(pProHW,pProslic->channel,DTMFDTF_B0_3,Si3226x_DTMFDec_Presets[preset].dtmfdtf_b0_3);
    WriteRAM(pProHW,pProslic->channel,DTMFDTF_B1_3,Si3226x_DTMFDec_Presets[preset].dtmfdtf_b1_3);
    WriteRAM(pProHW,pProslic->channel,DTMFDTF_B2_3,Si3226x_DTMFDec_Presets[preset].dtmfdtf_b2_3);
    WriteRAM(pProHW,pProslic->channel,DTMFDTF_A1_3,Si3226x_DTMFDec_Presets[preset].dtmfdtf_a1_3);
    WriteRAM(pProHW,pProslic->channel,DTMFDTF_A2_3,Si3226x_DTMFDec_Presets[preset].dtmfdtf_a2_3);
    return 0;
}
#endif


/*
** Function: PROSLIC_SetProfile
**
** Description: 
** set country profile of the proslic
*/
int Si3226x_SetProfile (proslicChanType *pProslic, int preset){
    /*TO DO
      Will be filled in at a later date*/
    return 0;
}

/*
** Function: PROSLIC_ZsynthSetup
**
** Description: 
** configure impedence synthesis
*/
#ifndef DISABLE_ZSYNTH_SETUP
int Si3226x_ZsynthSetup (proslicChanType *pProslic, int preset){
    uInt8 lf;
    uInt8 cal_en = 0;
    uInt16 timer = 500;

    lf = ReadReg(pProHW,pProslic->channel,LINEFEED);
    WriteReg(pProHW,pProslic->channel,LINEFEED,0);
    /*
    ** Load provided coefficients - these are presumed to be 0dB/0dB
    */
    WriteRAM(pProHW,pProslic->channel,TXACEQ_C0,Si3226x_Impedance_Presets[preset].audioEQ.txaceq_c0);
    WriteRAM(pProHW,pProslic->channel,TXACEQ_C1,Si3226x_Impedance_Presets[preset].audioEQ.txaceq_c1);
    WriteRAM(pProHW,pProslic->channel,TXACEQ_C2,Si3226x_Impedance_Presets[preset].audioEQ.txaceq_c2);
    WriteRAM(pProHW,pProslic->channel,TXACEQ_C3,Si3226x_Impedance_Presets[preset].audioEQ.txaceq_c3);
    WriteRAM(pProHW,pProslic->channel,RXACEQ_C0,Si3226x_Impedance_Presets[preset].audioEQ.rxaceq_c0);
    WriteRAM(pProHW,pProslic->channel,RXACEQ_C1,Si3226x_Impedance_Presets[preset].audioEQ.rxaceq_c1);
    WriteRAM(pProHW,pProslic->channel,RXACEQ_C2,Si3226x_Impedance_Presets[preset].audioEQ.rxaceq_c2);
    WriteRAM(pProHW,pProslic->channel,RXACEQ_C3,Si3226x_Impedance_Presets[preset].audioEQ.rxaceq_c3);
    WriteRAM(pProHW,pProslic->channel,ECFIR_C2,Si3226x_Impedance_Presets[preset].hybrid.ecfir_c2);
    WriteRAM(pProHW,pProslic->channel,ECFIR_C3,Si3226x_Impedance_Presets[preset].hybrid.ecfir_c3);
    WriteRAM(pProHW,pProslic->channel,ECFIR_C4,Si3226x_Impedance_Presets[preset].hybrid.ecfir_c4);
    WriteRAM(pProHW,pProslic->channel,ECFIR_C5,Si3226x_Impedance_Presets[preset].hybrid.ecfir_c5);
    WriteRAM(pProHW,pProslic->channel,ECFIR_C6,Si3226x_Impedance_Presets[preset].hybrid.ecfir_c6);
    WriteRAM(pProHW,pProslic->channel,ECFIR_C7,Si3226x_Impedance_Presets[preset].hybrid.ecfir_c7);
    WriteRAM(pProHW,pProslic->channel,ECFIR_C8,Si3226x_Impedance_Presets[preset].hybrid.ecfir_c8);
    WriteRAM(pProHW,pProslic->channel,ECFIR_C9,Si3226x_Impedance_Presets[preset].hybrid.ecfir_c9);
    WriteRAM(pProHW,pProslic->channel,ECIIR_B0,Si3226x_Impedance_Presets[preset].hybrid.ecfir_b0);
    WriteRAM(pProHW,pProslic->channel,ECIIR_B1,Si3226x_Impedance_Presets[preset].hybrid.ecfir_b1);
    WriteRAM(pProHW,pProslic->channel,ECIIR_A1,Si3226x_Impedance_Presets[preset].hybrid.ecfir_a1);
    WriteRAM(pProHW,pProslic->channel,ECIIR_A2,Si3226x_Impedance_Presets[preset].hybrid.ecfir_a2);
    WriteRAM(pProHW,pProslic->channel,ZSYNTH_A1,Si3226x_Impedance_Presets[preset].zsynth.zsynth_a1);
    WriteRAM(pProHW,pProslic->channel,ZSYNTH_A2,Si3226x_Impedance_Presets[preset].zsynth.zsynth_a2);
    WriteRAM(pProHW,pProslic->channel,ZSYNTH_B1,Si3226x_Impedance_Presets[preset].zsynth.zsynth_b1);
    WriteRAM(pProHW,pProslic->channel,ZSYNTH_B0,Si3226x_Impedance_Presets[preset].zsynth.zsynth_b0);
    WriteRAM(pProHW,pProslic->channel,ZSYNTH_B2,Si3226x_Impedance_Presets[preset].zsynth.zsynth_b2);
    WriteReg(pProHW,pProslic->channel,RA,Si3226x_Impedance_Presets[preset].zsynth.ra);
    WriteRAM(pProHW,pProslic->channel,TXACGAIN,Si3226x_Impedance_Presets[preset].txgain);
    WriteRAM(pProHW,pProslic->channel,RXACGAIN_SAVE,Si3226x_Impedance_Presets[preset].rxgain);
    WriteRAM(pProHW,pProslic->channel,RXACGAIN,Si3226x_Impedance_Presets[preset].rxgain);
    WriteRAM(pProHW,pProslic->channel,RXACHPF_B0_1,Si3226x_Impedance_Presets[preset].rxachpf_b0_1);
    WriteRAM(pProHW,pProslic->channel,RXACHPF_B1_1,Si3226x_Impedance_Presets[preset].rxachpf_b1_1);
    WriteRAM(pProHW,pProslic->channel,RXACHPF_A1_1,Si3226x_Impedance_Presets[preset].rxachpf_a1_1);

    /*
    ** Scale based on desired gain plan
    */
    Si3226x_dbgSetTXGain(pProslic,Si3226x_Impedance_Presets[preset].txgain_db,preset,TXACGAIN_SEL);
    Si3226x_dbgSetRXGain(pProslic,Si3226x_Impedance_Presets[preset].rxgain_db,preset,RXACGAIN_SEL);
    Si3226x_TXAudioGainSetup(pProslic,TXACGAIN_SEL);
    Si3226x_RXAudioGainSetup(pProslic,RXACGAIN_SEL);

    /* 
    ** Perform Zcal in case OHT used (eg. no offhook event to trigger auto Zcal) 
    */
    WriteReg(pProHW,pProslic->channel,CALR0,0x00);   
    WriteReg(pProHW,pProslic->channel,CALR1,0x40);   
    WriteReg(pProHW,pProslic->channel,CALR2,0x00); 
    WriteReg(pProHW,pProslic->channel,CALR3,0x80);  /* start cal */

    /* Wait for zcal to finish */
    do {
        cal_en = ReadReg(pProHW,pProslic->channel,CALR3);
        Delay(pProTimer,1);
        timer--;
    }while((cal_en&0x80)&&(timer>0));  
     
    WriteReg(pProHW,pProslic->channel,LINEFEED,lf);

    if(timer > 0) return 0;
    else          return RC_CAL_TIMEOUT;
}
#endif

/*
** Function: PROSLIC_GciCISetup
**
** Description: 
** configure CI bits (GCI mode)
*/
#ifndef DISABLE_CI_SETUP
int Si3226x_GciCISetup (proslicChanType *pProslic, int preset){
    WriteReg(pProHW,pProslic->channel,GCI_CI,Si3226x_CI_Presets[preset].gci_ci);
    return 0;
}
#endif
/*
** Function: PROSLIC_ModemDetSetup
**
** Description: 
** configure modem detector
*/
int Si3226x_ModemDetSetup (proslicChanType *pProslic, int preset){
    /*TO DO
      Will be filled in at a later date*/
    return 0;
}

/*
** Function: PROSLIC_AudioGainSetup
**
** Description: 
** configure audio gains
*/
int Si3226x_TXAudioGainSetup (proslicChanType *pProslic, int preset){
    WriteRAM(pProHW,pProslic->channel,TXACGAIN,Si3226x_audioGain_Presets[preset].acgain);
    WriteRAM(pProHW,pProslic->channel,TXACEQ_C0,Si3226x_audioGain_Presets[preset].aceq_c0);
    WriteRAM(pProHW,pProslic->channel,TXACEQ_C1,Si3226x_audioGain_Presets[preset].aceq_c1);
    WriteRAM(pProHW,pProslic->channel,TXACEQ_C2,Si3226x_audioGain_Presets[preset].aceq_c2);
    WriteRAM(pProHW,pProslic->channel,TXACEQ_C3,Si3226x_audioGain_Presets[preset].aceq_c3);
    return 0;
}

/*
** Function: PROSLIC_AudioGainSetup
**
** Description: 
** configure audio gains
*/
int Si3226x_RXAudioGainSetup (proslicChanType *pProslic, int preset){
    WriteRAM(pProHW,pProslic->channel,RXACGAIN_SAVE,Si3226x_audioGain_Presets[preset].acgain);
    WriteRAM(pProHW,pProslic->channel,RXACGAIN,Si3226x_audioGain_Presets[preset].acgain);
    WriteRAM(pProHW,pProslic->channel,RXACEQ_C0,Si3226x_audioGain_Presets[preset].aceq_c0);
    WriteRAM(pProHW,pProslic->channel,RXACEQ_C1,Si3226x_audioGain_Presets[preset].aceq_c1);
    WriteRAM(pProHW,pProslic->channel,RXACEQ_C2,Si3226x_audioGain_Presets[preset].aceq_c2);
    WriteRAM(pProHW,pProslic->channel,RXACEQ_C3,Si3226x_audioGain_Presets[preset].aceq_c3);
    return 0;
}




/*
** Function: PROSLIC_DCFeedSetup
**
** Description: 
** configure dc feed
*/
#ifndef DISABLE_DCFEED_SETUP
int Si3226x_DCFeedSetup (proslicChanType *pProslic, int preset){
    uInt8 lf;
    lf = ReadReg(pProHW,pProslic->channel,LINEFEED);
    WriteReg(pProHW,pProslic->channel,LINEFEED,0);
    WriteRAM(pProHW,pProslic->channel,SLOPE_VLIM,Si3226x_DCfeed_Presets[preset].slope_vlim);
    WriteRAM(pProHW,pProslic->channel,SLOPE_RFEED,Si3226x_DCfeed_Presets[preset].slope_rfeed);
    WriteRAM(pProHW,pProslic->channel,SLOPE_ILIM,Si3226x_DCfeed_Presets[preset].slope_ilim);
    WriteRAM(pProHW,pProslic->channel,SLOPE_DELTA1,Si3226x_DCfeed_Presets[preset].delta1);
    WriteRAM(pProHW,pProslic->channel,SLOPE_DELTA2,Si3226x_DCfeed_Presets[preset].delta2);
    WriteRAM(pProHW,pProslic->channel,V_VLIM,Si3226x_DCfeed_Presets[preset].v_vlim);
    WriteRAM(pProHW,pProslic->channel,V_RFEED,Si3226x_DCfeed_Presets[preset].v_rfeed);
    WriteRAM(pProHW,pProslic->channel,V_ILIM,Si3226x_DCfeed_Presets[preset].v_ilim);
    WriteRAM(pProHW,pProslic->channel,CONST_RFEED,Si3226x_DCfeed_Presets[preset].const_rfeed);
    WriteRAM(pProHW,pProslic->channel,CONST_ILIM,Si3226x_DCfeed_Presets[preset].const_ilim);
    WriteRAM(pProHW,pProslic->channel,I_VLIM,Si3226x_DCfeed_Presets[preset].i_vlim);
    WriteRAM(pProHW,pProslic->channel,LCRONHK,Si3226x_DCfeed_Presets[preset].lcronhk);
    WriteRAM(pProHW,pProslic->channel,LCROFFHK,Si3226x_DCfeed_Presets[preset].lcroffhk);
    WriteRAM(pProHW,pProslic->channel,LCRDBI,Si3226x_DCfeed_Presets[preset].lcrdbi);
    WriteRAM(pProHW,pProslic->channel,LONGHITH,Si3226x_DCfeed_Presets[preset].longhith);
    WriteRAM(pProHW,pProslic->channel,LONGLOTH,Si3226x_DCfeed_Presets[preset].longloth);
    WriteRAM(pProHW,pProslic->channel,LONGDBI,Si3226x_DCfeed_Presets[preset].longdbi);
    WriteRAM(pProHW,pProslic->channel,LCRMASK,Si3226x_DCfeed_Presets[preset].lcrmask);
    WriteRAM(pProHW,pProslic->channel,LCRMASK_POLREV,Si3226x_DCfeed_Presets[preset].lcrmask_polrev);
    WriteRAM(pProHW,pProslic->channel,LCRMASK_STATE,Si3226x_DCfeed_Presets[preset].lcrmask_state);
    WriteRAM(pProHW,pProslic->channel,LCRMASK_LINECAP,Si3226x_DCfeed_Presets[preset].lcrmask_linecap);
    WriteRAM(pProHW,pProslic->channel,VCM_OH,Si3226x_DCfeed_Presets[preset].vcm_oh);
    WriteRAM(pProHW,pProslic->channel,VOV_BAT,Si3226x_DCfeed_Presets[preset].vov_bat);
    WriteRAM(pProHW,pProslic->channel,VOV_GND,Si3226x_DCfeed_Presets[preset].vov_gnd);
    /* Overwrite vov_bat on fixed rail */
#ifdef SIVOICE_MULTI_BOM_SUPPORT
#define VOV_BAT_6V 0x624DD2L   /* 6v */
    if(Si3226x_General_Configuration.bomOpt == BO_DCDC_FIXED_RAIL)
    {
        WriteRAM(pProHW,pProslic->channel,VOV_BAT,VOV_BAT_6V);
    }
#endif
    WriteReg(pProHW,pProslic->channel,LINEFEED,lf);
    return 0;
}
#endif
/*
** Function: PROSLIC_GPIOSetup
**
** Description: 
** configure gpio
*/
#ifndef DISABLE_GPIO_SETUP
int Si3226x_GPIOSetup (proslicChanType *pProslic){
    uInt8 data;
    data = ReadReg(pProHW,pProslic->channel,GPIO);
    data |= Si3226x_GPIO_Configuration.outputEn << 4;
    WriteReg(pProHW,pProslic->channel,GPIO,data);
    data = Si3226x_GPIO_Configuration.analog << 4;
    data |= Si3226x_GPIO_Configuration.direction;
    WriteReg(pProHW,pProslic->channel,GPIO_CFG1,data);
    data = Si3226x_GPIO_Configuration.manual << 4;
    data |= Si3226x_GPIO_Configuration.polarity;
    WriteReg(pProHW,pProslic->channel,GPIO_CFG2,data);
    data |= Si3226x_GPIO_Configuration.openDrain;
    WriteReg(pProHW,pProslic->channel,GPIO_CFG3,data);
    return 0;
}
#endif

/*
** Function: PROSLIC_PulseMeterSetup
**
** Description: 
** configure pulse metering
*/
#ifndef DISABLE_PULSE_SETUP
int Si3226x_PulseMeterSetup (proslicChanType *pProslic, int preset){
    uInt8 reg;
    WriteRAM(pProHW,pProslic->channel,PM_AMP_THRESH,Si3226x_PulseMeter_Presets[preset].pm_amp_thresh);
    reg = (Si3226x_PulseMeter_Presets[preset].pmFreq<<1) | (Si3226x_PulseMeter_Presets[preset].pmRampRate<<4);
    WriteReg(pProHW,pProslic->channel,PMCON,reg);
    return 0;
}
#endif
/*
** Function: PROSLIC_PCMSetup
**
** Description: 
** configure pcm
*/
#ifndef DISABLE_PCM_SETUP
int Si3226x_PCMSetup (proslicChanType *pProslic, int preset){
    uInt8 regTemp;
        
    if (Si3226x_PCM_Presets[preset].widebandEn){
        regTemp = ReadReg(pProHW,pProslic->channel,DIGCON);
        WriteReg(pProHW,pProslic->channel,DIGCON,regTemp|0xC);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_B0_1,0x27EA83L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_B1_1,0x27EA83L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_A1_1,0x487977EL);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_B0_2,0x8000000L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_B1_2,0x7E8704DL);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_B2_2,0x8000000L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_A1_2,0x368C302L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_A2_2,0x18EBB1A4L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_B0_3,0x8000000L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_B1_3,0x254C75AL);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_B2_3,0x7FFFFFFL);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_A1_3,0x639A165L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_A2_3,0x1B6738A0L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_B0_1,0x4FD507L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_B1_1,0x4FD507L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_A1_1,0x487977EL);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_B0_2,0x8000000L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_B1_2,0x7E8704DL);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_B2_2,0x8000000L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_A1_2,0x368C302L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_A2_2,0x18EBB1A4L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_B0_3,0x8000000L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_B1_3,0x254C75AL);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_B2_3,0x7FFFFFFL);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_A1_3,0x639A165L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_A2_3,0x1B6738A0L);
        regTemp = ReadReg(pProHW,pProslic->channel,ENHANCE);
        WriteReg(pProHW,pProslic->channel,ENHANCE,regTemp|1);
    } else {
        regTemp = ReadReg(pProHW,pProslic->channel,DIGCON);
        WriteReg(pProHW,pProslic->channel,DIGCON,regTemp&~(0xC));
        WriteRAM(pProHW,pProslic->channel,TXACIIR_B0_1,0x3538E80L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_B1_1,0x3538E80L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_A1_1,0x1AA9100L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_B0_2,0x216D100L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_B1_2,0x2505400L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_B2_2,0x216D100L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_A1_2,0x2CB8100L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_A2_2,0x1D7FA500L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_B0_3,0x2CD9B00L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_B1_3,0x1276D00L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_B2_3,0x2CD9B00L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_A1_3,0x2335300L);
        WriteRAM(pProHW,pProslic->channel,TXACIIR_A2_3,0x19D5F700L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_B0_1,0x6A71D00L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_B1_1,0x6A71D00L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_A1_1,0x1AA9100L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_B0_2,0x216D100L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_B1_2,0x2505400L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_B2_2,0x216D100L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_A1_2,0x2CB8100L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_A2_2,0x1D7FA500L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_B0_3,0x2CD9B00L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_B1_3,0x1276D00L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_B2_3,0x2CD9B00L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_A1_3,0x2335300L);
        WriteRAM(pProHW,pProslic->channel,RXACIIR_A2_3,0x19D5F700L);
        regTemp = ReadReg(pProHW,pProslic->channel,ENHANCE);
        WriteReg(pProHW,pProslic->channel,ENHANCE,regTemp&~(1));
    }
    regTemp = Si3226x_PCM_Presets[preset].pcmFormat;
    regTemp |= Si3226x_PCM_Presets[preset].pcm_tri << 5;
    regTemp |= Si3226x_PCM_Presets[preset].alaw_inv << 2;
    WriteReg(pProHW,pProslic->channel,PCMMODE,regTemp);
    regTemp = ReadReg(pProHW,pProslic->channel,PCMTXHI);
    regTemp &= 3;
    regTemp |= Si3226x_PCM_Presets[preset].tx_edge<<4;
    WriteReg(pProHW,pProslic->channel,PCMTXHI,regTemp);

    return 0;
}
#endif
/*
** Function: PROSLIC_PCMSetup
**
** Description: 
** configure pcm
*/
int Si3226x_PCMTimeSlotSetup (proslicChanType *pProslic, uInt16 rxcount, uInt16 txcount){
    uInt8 data;
    data = txcount & 0xff;
    WriteReg(pProHW,pProslic->channel,PCMTXLO,data);
    data = txcount >> 8 ;
    WriteReg(pProHW,pProslic->channel,PCMTXHI,data);
    data = rxcount & 0xff;
    WriteReg(pProHW,pProslic->channel,PCMRXLO,data);
    data = rxcount >> 8 ;
    WriteReg(pProHW,pProslic->channel,PCMRXHI,data);

    return 0;
}

/*
**
** PROSLIC CONTROL FUNCTIONS
**
*/



/*
** Function: PROSLIC_GetInterrupts
**
** Description: 
** Reads interrupt registers status (IRQ1-4)
**
** Returns:
** array of pending interrupts of type proslicIntType*
**
*/
int Si3226x_GetInterrupts (proslicChanType *pProslic,proslicIntType *pIntData){
    /*Reading the interrupt registers and will clear any bits which are set (SPI mode only)
      Multiple interrupts may occur at once so bear that in mind when
      writing an interrupt handling routine*/
    uInt8 data[4];
    int i,j,k;
    pIntData->number = 0;


    if(pProslic->channelType != PROSLIC) {
      return RC_IGNORE;
    }
        
    data[0] = ReadReg(pProHW,pProslic->channel,IRQ1);
    data[1] = ReadReg(pProHW,pProslic->channel,IRQ2);
    data[2] = ReadReg(pProHW,pProslic->channel,IRQ3);
    data[3] = ReadReg(pProHW,pProslic->channel,IRQ4);
#ifdef GCI_MODE
    WriteReg(pProHW,pProslic->channel,IRQ1,data[0]); /*clear interrupts (gci only)*/
    WriteReg(pProHW,pProslic->channel,IRQ2,data[1]);
    WriteReg(pProHW,pProslic->channel,IRQ3,data[2]);
    WriteReg(pProHW,pProslic->channel,IRQ4,data[3]);
#endif
    for (i=0;i<4;i++){
        for (j=0;j<8;j++){
            if (data[i]&(1<<j)){
                switch (j + (i*8)){
                    /* IRQ 1 */
                case IRQ_OSC1_T1_SI3226X:   /* IRQ1.0 */
                    k=IRQ_OSC1_T1;
                    break;
                case IRQ_OSC1_T2_SI3226X:   /* IRQ1.1 */
                    k=IRQ_OSC1_T2;
                    break;
                case IRQ_OSC2_T1_SI3226X:   /* IRQ1.2 */
                    k=IRQ_OSC2_T1;
                    break;
                case IRQ_OSC2_T2_SI3226X:   /* IRQ1.3 */
                    k=IRQ_OSC2_T2;
                    break;
                case IRQ_RING_T1_SI3226X:   /* IRQ1.4 */
                    k=IRQ_RING_T1;
                    break;
                case IRQ_RING_T2_SI3226X:   /* IRQ1.5 */
                    k=IRQ_RING_T2;
                    break;
                case IRQ_FSKBUF_AVAIL_SI3226X:/* IRQ1.6 */
                    k=IRQ_FSKBUF_AVAIL;
                    break;
                case IRQ_VBAT_SI3226X:      /* IRQ1.7 */
                    k=IRQ_VBAT;
                    break;
                    /* IRQ2 */
                case IRQ_RING_TRIP_SI3226X: /* IRQ2.0 */
                    k=IRQ_RING_TRIP;
                    break;
                case IRQ_LOOP_STAT_SI3226X: /* IRQ2.1 */
                    k=IRQ_LOOP_STATUS;
                    break;
                case IRQ_LONG_STAT_SI3226X: /* IRQ2.2 */
                    k=IRQ_LONG_STAT;
                    break;
                case IRQ_VOC_TRACK_SI3226X: /* IRQ2.3 */
                    k=IRQ_VOC_TRACK;
                    break;
                case IRQ_DTMF_SI3226X:      /* IRQ2.4 */
                    k=IRQ_DTMF;
                    break;
                case IRQ_INDIRECT_SI3226X:  /* IRQ2.5 */
                    k=IRQ_INDIRECT;
                    break;
                case IRQ_TXMDM_SI3226X:     /* IRQ2.6 */
                    k = IRQ_TXMDM;
                    break;
                case IRQ_RXMDM_SI3226X:     /* IRQ2.7 */
                    k=IRQ_RXMDM;
                    break;
                    /* IRQ3 */
                case IRQ_P_HVIC_SI3226X:       /* IRQ3.0 */
                    k=IRQ_P_HVIC;
                    break;
                case IRQ_P_THERM_SI3226X:       /* IRQ3.1 */
                    k=IRQ_P_THERM;
                    break;
                case IRQ_PQ3_SI3226X:       /* IRQ3.2 */
                    k=IRQ_PQ3;  
                    break;
                case IRQ_PQ4_SI3226X:       /* IRQ3.3 */
                    k=IRQ_PQ4;
                    break;
                case IRQ_PQ5_SI3226X:       /* IRQ3.4 */
                    k=IRQ_PQ5;
                    break;
                case IRQ_PQ6_SI3226X:       /* IRQ3.5 */
                    k=IRQ_PQ6;
                    break;
                case IRQ_DSP_SI3226X:       /* IRQ3.6 */
                    k=IRQ_DSP;
                    break;
                case IRQ_MADC_FS_SI3226X:       /* IRQ3.7 */
                    k=IRQ_MADC_FS;
                    break;
                    /* IRQ4 */
                case IRQ_USER_0_SI3226X: /* IRQ4.0 */
                    k=IRQ_USER_0;
                    break;
                case IRQ_USER_1_SI3226X: /* IRQ4.1 */
                    k=IRQ_USER_1;
                    break;
                case IRQ_USER_2_SI3226X: /* IRQ4.2 */
                    k=IRQ_USER_2;
                    break;
                case IRQ_USER_3_SI3226X: /* IRQ4.3 */
                    k=IRQ_USER_3;
                    break;
                case IRQ_USER_4_SI3226X: /* IRQ4.4 */
                    k=IRQ_USER_4;
                    break;
                case IRQ_USER_5_SI3226X: /* IRQ4.5 */
                    k=IRQ_USER_5;
                    break;
                case IRQ_USER_6_SI3226X: /* IRQ4.6 */
                    k=IRQ_USER_6;
                    break;
                case IRQ_USER_7_SI3226X: /* IRQ4.7 */
                    k=IRQ_USER_7;
                    break;
                default:
                    k=0xff;
                }/* switch */
                pIntData->irqs[pIntData->number] =      k;              
                pIntData->number++;
                        
            }/* if */
        }/* for */      

    }

    return pIntData->number;
}


/*
** Function: PROSLIC_ReadHookStatus
**
** Description: 
** Determine hook status
*/
int Si3226x_ReadHookStatus (proslicChanType *pProslic,uInt8 *pHookStat){
    if (ReadReg(pProHW,pProslic->channel,LCRRTP) & 2)
        *pHookStat=PROSLIC_OFFHOOK;
    else
        *pHookStat=PROSLIC_ONHOOK;
    return 0;
}

/*
** Function: PROSLIC_WriteLinefeed
**
** Description: 
** Sets linefeed state
*/
int Si3226x_SetLinefeedStatus (proslicChanType *pProslic,uInt8 newLinefeed){
    uInt8 regTemp;
    WriteReg (pProHW, pProslic->channel, LINEFEED,newLinefeed);
    if ((newLinefeed&0xf) == LF_RINGING) {
        /*disable vbat interrupt during ringing*/
        regTemp = ReadReg(pProHW,pProslic->channel,IRQEN1);
        WriteReg (pProHW,pProslic->channel,IRQEN1,regTemp&(~0x80));
    }
    else{
        if (pProslic->deviceId->chipRev != 0) {
            regTemp = ReadReg(pProHW,pProslic->channel,IRQEN1);
            if (regTemp != 0)
                WriteReg (pProHW,pProslic->channel,IRQEN1,0x80 | regTemp);
        }
    }
    return 0;
}

/*
** Function: Si324x_SetLinefeedStatusBroadcast
**
** Description: 
** Sets linefeed state
*/
int Si3226x_SetLinefeedStatusBroadcast (proslicChanType *pProslic, uInt8 newLinefeed){

    WriteReg (pProHW, BROADCAST, LINEFEED,newLinefeed);
    return 0;
}


/*
** Function: PROSLIC_PolRev
**
** Description: 
** Sets polarity reversal state
*/
int Si3226x_PolRev (proslicChanType *pProslic,uInt8 abrupt, uInt8 newPolRevState){
    uInt8 data=0;
    switch (newPolRevState){
    case POLREV_STOP:
        data = 0;
        break;
    case POLREV_START:
        data = 2;
        break;
    case WINK_START:
        data = 6;
        break;
    case WINK_STOP:
        data = 4;
        break;
    }
    if (abrupt)
        data |= 1;
    WriteReg(pProHW,pProslic->channel,POLREV,data);
        
    return 0;
}

/*
** Function: PROSLIC_GPIOControl
**
** Description: 
** Sets gpio of the proslic
*/
int Si3226x_GPIOControl (proslicChanType *pProslic,uInt8 *pGpioData, uInt8 read){
    if (read)
        *pGpioData = 0xf & ReadReg(pProHW,pProslic->channel,GPIO);
    else{
        WriteReg(pProHW,pProslic->channel,GPIO,(*pGpioData)|(ReadReg(pProHW,pProslic->channel,GPIO)&0xf0));
    }
    return 0;
}

/*
** Function: PROSLIC_MWI
**
** Description: 
** implements message waiting indicator
*/
int Si3226x_MWI (proslicChanType *pProslic,uInt8 lampOn){
    /*message waiting (neon flashing) requires modifications to vbath_expect and slope_vlim.
      The old values are restored to turn off the lamp. We assume all channels set up the same.
      During off-hook event lamp must be disabled manually. */
    static int32 vbath_save = 0;
    static int32 slope_vlim_save = 0;
    uInt8 hkStat; int32 slope_vlim_tmp;
    slope_vlim_tmp = ReadRAM(pProHW,pProslic->channel,SLOPE_VLIM);
    Si3226x_ReadHookStatus(pProslic,&hkStat);

    if (lampOn && (hkStat == PROSLIC_OFFHOOK) ) {/*cant neon flash during offhook*/
#ifdef ENABLE_DEBUG
        if (pProslic->debugMode)        
            LOGPRINT ("Si3226x MWI cannot operate offhook\n");
#endif
        return RC_LINE_IN_USE;
    }

    if (lampOn) {
        if (slope_vlim_tmp != 0x8000000L) { /*check we're not already on*/
            vbath_save = ReadRAM(pProHW,pProslic->channel,VBATH_EXPECT);
            slope_vlim_save = slope_vlim_tmp;
        }
        WriteRAM(pProHW,pProslic->channel,VBATH_EXPECT,0x7AE147AL);/*120V*/
        WriteRAM(pProHW,pProslic->channel,SLOPE_VLIM,0x8000000L);
    } else {
        if (vbath_save != 0) { /*check we saved some valid value first*/
            WriteRAM(pProHW,pProslic->channel,VBATH_EXPECT,vbath_save);
            WriteRAM(pProHW,pProslic->channel,SLOPE_VLIM,slope_vlim_save);
        }
    }

    return RC_NONE;
}

/*
** Function: PROSLIC_StartGenericTone
**
** Description: 
** start tone generators
*/
int Si3226x_ToneGenStart (proslicChanType *pProslic,uInt8 timerEn){
    uInt8 data;
    data = ReadReg(pProHW,pProslic->channel,OCON);
    data |= 0x11 + (timerEn ? 0x66 : 0);
    WriteReg(pProHW,pProslic->channel,OCON,data);
    return 0;
}


/*
** Function: PROSLIC_StopTone
**
** Description: 
** Stops tone generators
**
** Input Parameters: 
** pProslic: pointer to Proslic object
**
** Return:
** none
*/
int Si3226x_ToneGenStop (proslicChanType *pProslic){
    uInt8 data;
#ifdef ENABLE_DEBUG
    if (pProslic->debugMode)
        LOGPRINT("Si3226x ToneGenStop\n");
#endif
    data = ReadReg(pProHW,pProslic->channel,OCON);
    data &= ~(0x77);
    WriteReg(pProHW,pProslic->channel,OCON,data);
    return 0;
}


/*
** Function: PROSLIC_StartRing
**
** Description: 
** start ring generator
*/
int Si3226x_RingStart (proslicChanType *pProslic){
#ifdef ENABLE_DEBUG
    if (pProslic->debugMode)
        LOGPRINT("Si3226x RingStart\n");
#endif
    Si3226x_SetLinefeedStatus(pProslic,LF_RINGING);
    return 0;
}


/*
** Function: PROSLIC_StopRing
**
** Description: 
** Stops ring generator
*/
int Si3226x_RingStop (proslicChanType *pProslic){
#ifdef ENABLE_DEBUG
    if (pProslic->debugMode)
        LOGPRINT("Si3226x RingStop\n");
#endif
    Si3226x_SetLinefeedStatus(pProslic,LF_FWD_ACTIVE);
    return 0;
}

/*
** Function: PROSLIC_EnableCID
**
** Description: 
** enable fsk
*/
int Si3226x_EnableCID (proslicChanType *pProslic){
    uInt8 data;
#ifdef ENABLE_DEBUG
    if (pProslic->debugMode)
        LOGPRINT("Si3226x EnableCID\n");
#endif
    WriteReg(pProHW,pProslic->channel,OCON,0);

    data = ReadReg(pProHW,pProslic->channel,OMODE);
    data |= 0xA;
    WriteReg(pProHW,pProslic->channel,OMODE,data);

    WriteReg(pProHW,pProslic->channel,OCON,0x5);
    return 0;
}

/*
** Function: PROSLIC_DisableCID
**
** Description: 
** disable fsk
*/
int Si3226x_DisableCID (proslicChanType *pProslic){
    uInt8 data;
#ifdef ENABLE_DEBUG
    if (pProslic->debugMode)
        LOGPRINT("Si3226x DisableCID\n");
#endif
    WriteReg(pProHW,pProslic->channel,OCON,0);
    data = ReadReg(pProHW,pProslic->channel,OMODE);
    data &= ~(0x8);
    WriteReg(pProHW,pProslic->channel,OMODE,data);
    return 0;
}

/*
** Function: PROSLIC_SendCID
**
** Description: 
** send fsk data
*/
int Si3226x_SendCID (proslicChanType *pProslic, uInt8 *buffer, uInt8 numBytes){
#ifdef ENABLE_DEBUG
    if (pProslic->debugMode)
        LOGPRINT("Si3226x SendCID\n");
#endif
    while (numBytes-- > 0){
        WriteReg(pProHW,pProslic->channel,FSKDAT,*(buffer++));
    }
    return 0;
}

/*
** Function: PROSLIC_StartPCM
**
** Description: 
** Starts PCM
*/
int Si3226x_PCMStart (proslicChanType *pProslic){
    uInt8 data;
#ifdef ENABLE_DEBUG
    if (pProslic->debugMode)
        LOGPRINT("Si3226x PCMStart\n");
#endif
    data = ReadReg(pProHW,pProslic->channel,PCMMODE);
    data |= 0x10;
    WriteReg(pProHW,pProslic->channel,PCMMODE,data);
    return 0;
}


/*
** Function: PROSLIC_StopPCM
**
** Description: 
** Disables PCM
*/
int Si3226x_PCMStop (proslicChanType *pProslic){
    uInt8 data;
#ifdef ENABLE_DEBUG
    if (pProslic->debugMode)
        LOGPRINT("Si3226x PCMStop\n");
#endif
    data = ReadReg(pProHW,pProslic->channel,PCMMODE);
    data &= ~(0x10);
    WriteReg(pProHW,pProslic->channel,PCMMODE,data);
    return 0;
}



/*
** Function: PROSLIC_ReadDTMFDigit
**
** Description: 
** Read DTMF digit (would be called after DTMF interrupt to collect digit)
*/
int Si3226x_DTMFReadDigit (proslicChanType *pProslic,uInt8 *pDigit){

    *pDigit = ReadReg(pProHW,pProslic->channel,TONDTMF) & 0xf;
#ifdef ENABLE_DEBUG
    if (pProslic->debugMode)
        LOGPRINT("Si3226x: DTMFReadDigit %d\n",*pDigit);
#endif
        
    return 0;
}

/*
** Function: PROSLIC_PLLFreeRunStart
**
** Description: 
** initiates pll free run mode
*/
int Si3226x_PLLFreeRunStart (proslicChanType *pProslic){
    uInt8 tmp;
    tmp = ReadReg(pProHW,pProslic->channel,ENHANCE);
    WriteReg(pProHW,pProslic->channel,ENHANCE,tmp|0x4);
    return 0;
}

/*
** Function: PROSLIC_PLLFreeRunStop
**
** Description: 
** exit pll free run mode
*/
int Si3226x_PLLFreeRunStop (proslicChanType *pProslic){
    uInt8 tmp;
    tmp = ReadReg(pProHW,pProslic->channel,ENHANCE);
    WriteReg(pProHW,pProslic->channel,ENHANCE,tmp&~(0x4));
    return 0;
}

/*
** Function: PROSLIC_PulseMeterStart
**
** Description: 
** start pulse meter tone
*/
int Si3226x_PulseMeterStart (proslicChanType *pProslic){

    WriteReg(pProHW,pProslic->channel,PMCON,ReadReg(pProHW,pProslic->channel,PMCON) | (0x5));
    return 0;   
}

/*
** Function: PROSLIC_PulseMeterStop
**
** Description: 
** stop pulse meter tone
*/
int Si3226x_PulseMeterStop (proslicChanType *pProslic){
 
    WriteReg(pProHW,pProslic->channel,PMCON,ReadReg(pProHW,pProslic->channel,PMCON) & ~(0x5));
    return 0;
}

/*
** Function: PROSLIC_dbgSetDCFeed
**
** Description: 
** provisionary function for setting up
** dcfeed given desired open circuit voltage 
** and loop current.
*/
int Si3226x_dbgSetDCFeed (proslicChanType *pProslic, uInt32 v_vlim_val, uInt32 i_ilim_val, int32 preset){
/* Note:  * needs more descriptive return codes in the event of an out of range arguement */
    uInt16 vslope = 160;
    uInt16 rslope = 720;
    uInt32 vscale1 = 1386; 
    uInt32 vscale2 = 1422;   /* 1386x1422 = 1970892 broken down to minimize trunc err */
    uInt32 iscale1 = 913;
    uInt32 iscale2 = 334;    /* 913x334 = 304942 */
    uInt32 i_rfeed_val, v_rfeed_val, const_rfeed_val, i_vlim_val, const_ilim_val, v_ilim_val;
    int32 signedVal;
    /* Set Linefeed to open state before modifying DC Feed */

    /* Assumptions must be made to minimize computations.  This limits the
    ** range of available settings, but should be more than adequate for
    ** short loop applications.
    **
    ** Assumtions:
    ** 
    ** SLOPE_VLIM      =>  160ohms
    ** SLOPE_RFEED     =>  720ohms
    ** I_RFEED         =>  3*I_ILIM/4
    ** 
    ** With these assumptions, the DC Feed parameters now become 
    **
    ** Inputs:      V_VLIM, I_ILIM
    ** Constants:   SLOPE_VLIM, SLOPE_ILIM, SLOPE_RFEED, SLOPE_DELTA1, SLOPE_DELTA2
    ** Outputs:     V_RFEED, V_ILIM, I_VLIM, CONST_RFEED, CONST_ILIM
    **
    */

    /* Validate arguements */
    if((i_ilim_val < 15)||(i_ilim_val > 45)) return 1;   /* need error code */
    if((v_vlim_val < 30)||(v_vlim_val > 52)) return 1;   /* need error code */

    /* Calculate voltages in mV and currents in uA */
    v_vlim_val *= 1000;
    i_ilim_val *= 1000;

    /* I_RFEED */
    i_rfeed_val = (3*i_ilim_val)/4;

    /* V_RFEED */
    v_rfeed_val = v_vlim_val - (i_rfeed_val*vslope)/1000;

    /* V_ILIM */ 
    v_ilim_val = v_rfeed_val - (rslope*(i_ilim_val - i_rfeed_val))/1000;

    /* I_VLIM */
    i_vlim_val = (v_vlim_val*1000)/4903;

    /* CONST_RFEED */
    signedVal = v_rfeed_val * (i_ilim_val - i_rfeed_val);
    signedVal /= (v_rfeed_val - v_ilim_val);
    signedVal = i_rfeed_val + signedVal;

    /* signedVal in uA here */
    signedVal *= iscale1;
    signedVal /= 100;
    signedVal *= iscale2;
    signedVal /= 10;

    if(signedVal < 0)
    {
        const_rfeed_val = (signedVal)+ (1L<<29);
    }
    else
    {
        const_rfeed_val = signedVal & 0x1FFFFFFF;
    }

    /* CONST_ILIM */
    const_ilim_val = i_ilim_val;

    /* compute RAM values */
    v_vlim_val *= vscale1;
    v_vlim_val /= 100;
    v_vlim_val *= vscale2;
    v_vlim_val /= 10;

    v_rfeed_val *= vscale1;
    v_rfeed_val /= 100;
    v_rfeed_val *= vscale2;
    v_rfeed_val /= 10;

    v_ilim_val *= vscale1;
    v_ilim_val /= 100;
    v_ilim_val *= vscale2;
    v_ilim_val /= 10;

    const_ilim_val *= iscale1;
    const_ilim_val /= 100;
    const_ilim_val *= iscale2;
    const_ilim_val /= 10;

    i_vlim_val *= iscale1;
    i_vlim_val /= 100;
    i_vlim_val *= iscale2;
    i_vlim_val /= 10;

    Si3226x_DCfeed_Presets[preset].slope_vlim = 0x18842BD7L;
    Si3226x_DCfeed_Presets[preset].slope_rfeed = 0x1E8886DEL;
    Si3226x_DCfeed_Presets[preset].slope_ilim = 0x40A0E0L;
    Si3226x_DCfeed_Presets[preset].delta1 = 0x1EABA1BFL;
    Si3226x_DCfeed_Presets[preset].delta2 = 0x1EF744EAL;
    Si3226x_DCfeed_Presets[preset].v_vlim = v_vlim_val;
    Si3226x_DCfeed_Presets[preset].v_rfeed = v_rfeed_val;
    Si3226x_DCfeed_Presets[preset].v_ilim = v_ilim_val;
    Si3226x_DCfeed_Presets[preset].const_rfeed = const_rfeed_val;
    Si3226x_DCfeed_Presets[preset].const_ilim = const_ilim_val;
    Si3226x_DCfeed_Presets[preset].i_vlim = i_vlim_val;
        
    return 0;
}

/*
** Function: PROSLIC_dbgSetDCFeedVopen
**
** Description: 
** provisionary function for setting up
** dcfeed given desired open circuit voltage.
** Entry I_ILIM value will be used.
*/
int Si3226x_dbgSetDCFeedVopen (proslicChanType *pProslic, uInt32 v_vlim_val, int32 preset)
{
    uInt32 i_ilim_val;
    uInt32 iscale1 = 913;
    uInt32 iscale2 = 334;    /* 913x334 = 304942 */

    /* Read present CONST_ILIM value */
    i_ilim_val = Si3226x_DCfeed_Presets[preset].const_ilim;


    i_ilim_val /= iscale2;
    i_ilim_val /= iscale1;

    return Si3226x_dbgSetDCFeed(pProslic,v_vlim_val,i_ilim_val,preset);
}

/*
** Function: PROSLIC_dbgSetDCFeedIloop
**
** Description: 
** provisionary function for setting up
** dcfeed given desired loop current.
** Entry V_VLIM value will be used.
*/
int Si3226x_dbgSetDCFeedIloop (proslicChanType *pProslic, uInt32 i_ilim_val, int32 preset)
{
    uInt32 v_vlim_val;
    uInt32 vscale1 = 1386; 
    uInt32 vscale2 = 1422;   /* 1386x1422 = 1970892 broken down to minimize trunc err */

    /* Read present V_VLIM value */
    v_vlim_val = Si3226x_DCfeed_Presets[preset].v_vlim;

    v_vlim_val /= vscale2;
    v_vlim_val /= vscale1;

    return Si3226x_dbgSetDCFeed(pProslic,v_vlim_val,i_ilim_val, preset);
}




typedef struct
{
    uInt8   freq;
    ramData ringfr;      /* trise scale for trap */
    uInt32  ampScale;
} ProSLIC_SineRingFreqLookup;

typedef struct
{
    uInt8    freq;
    ramData  rtacth;
    ramData rtper;
    ramData rtdb;
} ProSLIC_SineRingtripLookup;

typedef struct
{
    uInt8   freq;
    uInt16  cfVal[6];
} ProSLIC_TrapRingFreqLookup;

typedef struct
{
    uInt8   freq;
    ramData rtper;
    ramData rtdb;
    uInt32  rtacth[6];
} ProSLIC_TrapRingtripLookup;




/*
** Function: PROSLIC_dbgRingingSetup
**
** Description: 
** Provisionary function for setting up
** Ring type, frequency, amplitude and dc offset.
** Main use will be by peek/poke applications.
*/
int Si3226x_dbgSetRinging (proslicChanType *pProslic, ProSLIC_dbgRingCfg *ringCfg, int preset){
    int errVal,i=0;
    uInt32 vScale = 1608872L;   /* (2^28/170.25)*((100+4903)/4903) */
    ramData dcdcVminTmp;

    const ProSLIC_SineRingFreqLookup sineRingFreqTable[] =
/*  Freq RINGFR, vScale */
        {{15, 0x7F6E930L, 18968L},
         {16, 0x7F5A8E0L, 20234L},
         {20, 0x7EFD9D5L, 25301L},
         {22, 0x7EC770AL, 27843L},
         {23, 0x7EAA6E2L, 29113L},
         {25, 0x7E6C925L, 31649L},
         {30, 0x7DBB96BL, 38014L},
         {34, 0x7D34155L, 42270L}, /* Actually 33.33Hz */
         {35, 0x7CEAD72L, 44397L},
         {40, 0x7BFA887L, 50802L},
         {45, 0x7AEAE74L, 57233L},
         {50, 0x79BC384L, 63693L},
         {0,0,0}}; /* terminator */

    const ProSLIC_SineRingtripLookup sineRingtripTable[] =
/*  Freq rtacth */
        { {15, 11440000L, 0x6A000L, 0x4000L },
          {16, 10810000L, 0x64000L, 0x4000L },
          {20, 8690000L,  0x50000L, 0x8000L }, 
          {22, 7835000L,  0x48000L, 0x8000L },
          {23, 7622000L,  0x46000L, 0x8000L }, 
          {25, 6980000L,  0x40000L, 0xA000L }, 
          {30, 5900000L,  0x36000L, 0xA000L }, 
          {34, 10490000L, 0x60000L, 0x6000L }, /* Actually 33.33 */
          {35, 10060000L, 0x5C000L, 0x6000L }, 
          {40, 8750000L,  0x50000L, 0x8000L }, 
          {45, 7880000L,  0x48000L, 0x8000L }, 
          {50, 7010000L,  0x40000L, 0xA000L }, 
          {0,0L}}; /* terminator */

    const ProSLIC_TrapRingFreqLookup trapRingFreqTable[] =
/*  Freq multCF11 multCF12 multCF13 multCF14 multCF15 multCF16*/
    {
        {15, {69,122, 163, 196, 222,244}},
        {16, {65,115, 153, 184, 208,229}},
        {20, {52,92, 122, 147, 167,183}},
        {22, {47,83, 111, 134, 152,166}},
        {23, {45,80, 107, 128, 145,159}},
        {25, {42,73, 98, 118, 133,146}},
        {30, {35,61, 82, 98, 111,122}},
        {34, {31,55, 73, 88, 100,110}},
        {35, {30,52, 70, 84, 95,104}},
        {40, {26,46, 61, 73, 83,91}},
        {45, {23,41, 54, 65, 74,81}},
        {50, {21,37, 49, 59, 67,73}},
        {0,{0L,0L,0L,0L}} /* terminator */
    }; 


    const ProSLIC_TrapRingtripLookup trapRingtripTable[] =
/*  Freq rtper rtdb rtacthCR11 rtacthCR12 rtacthCR13 rtacthCR14 rtacthCR15 rtacthCR16*/
    {
        {15, 0x6A000L,  0x4000L, {16214894L, 14369375L, 12933127L, 11793508L, 10874121L, 10121671L}},
        {16, 0x64000L,  0x4000L, {15201463L, 13471289L, 12124806L, 11056414L, 10194489L, 9489067L}},
        {20, 0x50000L,  0x6000L, {12161171L, 10777031L, 9699845L, 8845131L, 8155591L, 7591253L}},
        {22, 0x48000L,  0x6000L, {11055610L, 9797301L, 8818041L, 8041028L, 7414174L, 6901139L}},
        {23, 0x46000L,  0x6000L, {10574931L, 9371331L, 8434648L, 7691418L, 7091818L, 6601090L}},
        {25, 0x40000L,  0x8000L, {9728937L, 8621625L, 7759876L, 7076105L, 6524473L, 6073003L}},
        {30, 0x36000L,  0x8000L, {8107447L, 7184687L, 6466563L, 5896754L, 5437061L, 5060836L}},
        {34, 0x60000L,  0x6000L, {7297432L, 6466865L, 5820489L, 5307609L, 4893844L, 4555208L}},
        {35, 0x5C000L,  0x6000L, {6949240L, 6158303L, 5542769L, 5054361L, 4660338L, 4337859L}},
        {40, 0x50000L,  0x6000L, {6080585L, 5388516L, 4849923L, 4422565L, 4077796L, 3795627L}},
        {45, 0x48000L,  0x6000L, {5404965L, 4789792L, 4311042L, 3931169L, 3624707L, 3373890L}},
        {50, 0x40000L,  0x8000L, {4864468L, 4310812L, 3879938L, 3538052L, 3262236L, 3036501L}},
        {0,0x0L, 0x0L, {0L,0L,0L,0L}} /* terminator */
    }; 

    errVal = 0;

    switch(ringCfg->ringtype)
    {
    case ProSLIC_RING_SINE:
        i=0;
        do
        {
            if(sineRingFreqTable[i].freq >= ringCfg->freq) 
            {
                break;
            }
            i++;
        } while (sineRingFreqTable[i].freq);

        /* Set to maximum value if exceeding maximum value from table */
        if(sineRingFreqTable[i].freq == 0)
        {
            i--;
            errVal = 1;
        }

        /* Update RINGFR RINGAMP, RINGOFFSET, and RINGCON */
        Si3226x_Ring_Presets[preset].freq = sineRingFreqTable[i].ringfr;
        Si3226x_Ring_Presets[preset].amp = ringCfg->amp * sineRingFreqTable[i].ampScale;
        Si3226x_Ring_Presets[preset].offset = ringCfg->offset * vScale;
        Si3226x_Ring_Presets[preset].phas = 0L;

        /* Don't alter anything in RINGCON other than clearing the TRAP bit */
        Si3226x_Ring_Presets[preset].ringcon &= 0xFE;

        Si3226x_Ring_Presets[preset].rtper = sineRingtripTable[i].rtper;
        Si3226x_Ring_Presets[preset].rtacdb = sineRingtripTable[i].rtdb;
        Si3226x_Ring_Presets[preset].rtdcdb = sineRingtripTable[i].rtdb;
        Si3226x_Ring_Presets[preset].rtdcth = 0xFFFFFFFL;
        Si3226x_Ring_Presets[preset].rtacth = sineRingtripTable[i].rtacth;
        break;

    case ProSLIC_RING_TRAP_CF11:  
    case ProSLIC_RING_TRAP_CF12:     
    case ProSLIC_RING_TRAP_CF13: 
    case ProSLIC_RING_TRAP_CF14: 
    case ProSLIC_RING_TRAP_CF15:  
    case ProSLIC_RING_TRAP_CF16:  
        i=0;
        do
        {
            if(trapRingFreqTable[i].freq >= ringCfg->freq) 
            {
                break;
            }
            i++;
        } while (trapRingFreqTable[i].freq);

        /* Set to maximum value if exceeding maximum value from table */
        if(trapRingFreqTable[i].freq == 0)
        {
            i--;
            errVal = 1;
        }

        /* Update RINGFR RINGAMP, RINGOFFSET, and RINGCON */
        Si3226x_Ring_Presets[preset].amp = ringCfg->amp * vScale;
        Si3226x_Ring_Presets[preset].freq = Si3226x_Ring_Presets[preset].amp/trapRingFreqTable[i].cfVal[ringCfg->ringtype];
        Si3226x_Ring_Presets[preset].offset = ringCfg->offset * vScale;
        Si3226x_Ring_Presets[preset].phas = 262144000L/trapRingFreqTable[i].freq;

        /* Don't alter anything in RINGCON other than setting the TRAP bit */
        Si3226x_Ring_Presets[preset].ringcon |= 0x01; 

        /* RTPER and debouce timers  */
        Si3226x_Ring_Presets[preset].rtper = trapRingtripTable[i].rtper;
        Si3226x_Ring_Presets[preset].rtacdb = trapRingtripTable[i].rtdb;
        Si3226x_Ring_Presets[preset].rtdcdb = trapRingtripTable[i].rtdb;  


        Si3226x_Ring_Presets[preset].rtdcth = 0xFFFFFFFL;
        Si3226x_Ring_Presets[preset].rtacth = trapRingtripTable[i].rtacth[ringCfg->ringtype];


        break;
    }

    /* 
    ** DCDC tracking sluggish under light load at higher ring freq.
    ** Reduce tracking depth above 40Hz.  This should have no effect
    ** if using the Buck-Boost architecture.
    */
    if((sineRingFreqTable[i].freq >= 40)||(Si3226x_General_Configuration.bomOpt == BO_DCDC_BUCK_BOOST))
    {
        dcdcVminTmp = ringCfg->amp + ringCfg->offset;
        dcdcVminTmp *= 1000;
        dcdcVminTmp *= SCALE_V_MADC;
        Si3226x_Ring_Presets[preset].vbat_track_min_rng = dcdcVminTmp;
    }
    else
    {
        Si3226x_Ring_Presets[preset].vbat_track_min_rng = 0x1800000L;
    }

    return errVal;
}


typedef struct
{
    int32   gain;
    uInt32 scale;
} ProSLIC_GainScaleLookup;

/*to be verified*/
#define GAIN_DELTA_MAX 6
#define GAIN_DELTA_MIN -12
#define GAIN_MAX 6
#define GAIN_MIN -12
#define MAX_AC_GAIN 3
#define MIN_AC_GAIN -6


static int Si3226x_dbgSetGain (proslicChanType *pProslic, int32 gain, int impedance_preset, int tx_rx_sel){
    int errVal = 0;
    int32 i;
    /*we adjust the gain starting from the impedance preset specified
      and store the results in the audio gain preset */

    int32 gain_1, gain_2;
    const ProSLIC_GainScaleLookup gainScaleTable[] =
/*  gain, scale=10^(gain/20) */
        { {-6, 501},
          {-5, 562},
          {-4, 631},
          {-3, 708},
          {-2, 794},
          {-1, 891},
          {0, 1000},
          {1, 1122},
          {2, 1259},
          {3, 1413},
          {4, 1585},
          {5, 1778},
          {6, 1995},
          {0xff,0}}; /* terminator */
 
    /* gain_delta = ((int32)gain) - Si3226x_Impedance_Presets[impedance_preset].txgain_db; */
/* 
** 5.4.0 - Removed relative gain scaling. to support automatic adjustment based on
**         gain plan provided in txgain_db and rxgain_db.  It is presumed that all
**         coefficients were generated for 0dB/0dB gain and the txgain_db and rxgain_db
**         parameters will be used to scale the gain using the existing gain provisioning
**         infrastructure when the zsynth preset is loaded.  This function will ignore 
**         the txgain_db and rxgain_db parameters and scale absolute gain presuming a
**         0dB/0dB coefficient set.
*/

    /* Validate desired gain is within range of this func */
    if (gain > GAIN_MAX || gain < GAIN_MIN)
        errVal = RC_GAIN_OUT_OF_RANGE;
   
    /* Split gain between sinc filter and equalizer */     
    gain_1 = gain;
    gain_2 = 0;

    if (gain > MAX_AC_GAIN)
    {
        gain_1 = MAX_AC_GAIN;
        gain_2 = gain - MAX_AC_GAIN;
    }
    if (gain < MIN_AC_GAIN)
    {
        gain_1 = MIN_AC_GAIN;
        gain_2 = gain - MIN_AC_GAIN;
    }


    /* Search scale factor lookup table for appropriate gain_1 */
    i=0;
    do
    {
        if(gainScaleTable[i].gain >= gain_1) 
        {
                
            break;
        }
        i++;
    } while (gainScaleTable[i].gain!=0xff);

    /* Set to maximum value if exceeding maximum value from table */
    if(gainScaleTable[i].gain == 0xff)
    {
        i--;
        errVal = RC_GAIN_DELTA_TOO_LARGE;
    }

    if(tx_rx_sel == TXACGAIN_SEL)
    {
        Si3226x_audioGain_Presets[0].acgain = (Si3226x_Impedance_Presets[impedance_preset].txgain/1000)*gainScaleTable[i].scale;
    }
    else
    {
        Si3226x_audioGain_Presets[1].acgain = (Si3226x_Impedance_Presets[impedance_preset].rxgain/1000)*gainScaleTable[i].scale;
    }


    /* Search scale factor lookup table for appropriate gain_2 */
    i=0;
    do
    {
        if(gainScaleTable[i].gain >= gain_2) 
        {
            break;
        }
        i++;
    } while (gainScaleTable[i].gain!=0xff);

    /* Set to maximum value if exceeding maximum value from table */
    if(gainScaleTable[i].gain == 0xff)
    {
        i--;
        errVal = RC_GAIN_DELTA_TOO_LARGE;
    }

    if(tx_rx_sel == TXACGAIN_SEL)
    {
        /*sign extend negative numbers*/
        if (Si3226x_Impedance_Presets[impedance_preset].audioEQ.txaceq_c0 & 0x10000000L)
            Si3226x_Impedance_Presets[impedance_preset].audioEQ.txaceq_c0 |= 0xf0000000L;
        if (Si3226x_Impedance_Presets[impedance_preset].audioEQ.txaceq_c1 & 0x10000000L)
            Si3226x_Impedance_Presets[impedance_preset].audioEQ.txaceq_c1 |= 0xf0000000L;
        if (Si3226x_Impedance_Presets[impedance_preset].audioEQ.txaceq_c2 & 0x10000000L)
            Si3226x_Impedance_Presets[impedance_preset].audioEQ.txaceq_c2 |= 0xf0000000L;
        if (Si3226x_Impedance_Presets[impedance_preset].audioEQ.txaceq_c3 & 0x10000000L)
            Si3226x_Impedance_Presets[impedance_preset].audioEQ.txaceq_c3 |= 0xf0000000L;

        Si3226x_audioGain_Presets[0].aceq_c0 = ((int32)Si3226x_Impedance_Presets[impedance_preset].audioEQ.txaceq_c0/1000)*gainScaleTable[i].scale;
        Si3226x_audioGain_Presets[0].aceq_c1 = ((int32)Si3226x_Impedance_Presets[impedance_preset].audioEQ.txaceq_c1/1000)*gainScaleTable[i].scale;
        Si3226x_audioGain_Presets[0].aceq_c2 = ((int32)Si3226x_Impedance_Presets[impedance_preset].audioEQ.txaceq_c2/1000)*gainScaleTable[i].scale;
        Si3226x_audioGain_Presets[0].aceq_c3 = ((int32)Si3226x_Impedance_Presets[impedance_preset].audioEQ.txaceq_c3/1000)*gainScaleTable[i].scale;
    }
    else
    {
        /*sign extend negative numbers*/
        if (Si3226x_Impedance_Presets[impedance_preset].audioEQ.rxaceq_c0 & 0x10000000L)
            Si3226x_Impedance_Presets[impedance_preset].audioEQ.rxaceq_c0 |= 0xf0000000L;
        if (Si3226x_Impedance_Presets[impedance_preset].audioEQ.rxaceq_c1 & 0x10000000L)
            Si3226x_Impedance_Presets[impedance_preset].audioEQ.rxaceq_c1 |= 0xf0000000L;
        if (Si3226x_Impedance_Presets[impedance_preset].audioEQ.rxaceq_c2 & 0x10000000L)
            Si3226x_Impedance_Presets[impedance_preset].audioEQ.rxaceq_c2 |= 0xf0000000L;
        if (Si3226x_Impedance_Presets[impedance_preset].audioEQ.rxaceq_c3 & 0x10000000L)
            Si3226x_Impedance_Presets[impedance_preset].audioEQ.rxaceq_c3 |= 0xf0000000L;

        Si3226x_audioGain_Presets[1].aceq_c0 = ((int32)Si3226x_Impedance_Presets[impedance_preset].audioEQ.rxaceq_c0/1000)*gainScaleTable[i].scale;
        Si3226x_audioGain_Presets[1].aceq_c1 = ((int32)Si3226x_Impedance_Presets[impedance_preset].audioEQ.rxaceq_c1/1000)*gainScaleTable[i].scale;
        Si3226x_audioGain_Presets[1].aceq_c2 = ((int32)Si3226x_Impedance_Presets[impedance_preset].audioEQ.rxaceq_c2/1000)*gainScaleTable[i].scale;
        Si3226x_audioGain_Presets[1].aceq_c3 = ((int32)Si3226x_Impedance_Presets[impedance_preset].audioEQ.rxaceq_c3/1000)*gainScaleTable[i].scale;
    }


    return errVal;
}

/*
** Function: PROSLIC_dbgSetTXGain
**
** Description: 
** Provisionary function for setting up
** TX gain
*/
int Si3226x_dbgSetTXGain (proslicChanType *pProslic, int32 gain, int impedance_preset, int audio_gain_preset){
    return Si3226x_dbgSetGain(pProslic,gain,impedance_preset,audio_gain_preset);
}

/*
** Function: PROSLIC_dbgSetRXGain
**
** Description: 
** Provisionary function for setting up
** RX gain
*/
int Si3226x_dbgSetRXGain (proslicChanType *pProslic, int32 gain, int impedance_preset, int audio_gain_preset){
    return Si3226x_dbgSetGain(pProslic,gain,impedance_preset,audio_gain_preset);
}


/*
** Function: Si3226x_LineMonitor
**
** Description: 
** Monitor line voltages and currents
*/
int Si3226x_LineMonitor(proslicChanType *pProslic, proslicMonitorType *monitor)
{
    if(pProslic->channelEnable)
    {
        monitor->vtr    = ReadRAM(pProHW,pProslic->channel,VDIFF_FILT);
        if(monitor->vtr & 0x10000000L)
            monitor->vtr |= 0xf0000000L;
        monitor->vtr /= SCALE_V_MADC;

        monitor->vtip    = ReadRAM(pProHW,pProslic->channel,VTIP);
        if(monitor->vtip & 0x10000000L)
            monitor->vtip |= 0xf0000000L;
        monitor->vtip /= SCALE_V_MADC;

        monitor->vring    = ReadRAM(pProHW,pProslic->channel,VRING);
        if(monitor->vring & 0x10000000L)
            monitor->vring |= 0xf0000000L;
        monitor->vring /= SCALE_V_MADC;

        monitor->vbat    = ReadRAM(pProHW,pProslic->channel,MADC_VBAT);
        if(monitor->vbat & 0x10000000L)
            monitor->vbat |= 0xf0000000L;
        monitor->vbat /= SCALE_V_MADC;

        monitor->itr  = ReadRAM(pProHW,pProslic->channel,MADC_ILOOP);
        if(monitor->itr & 0x10000000L)
            monitor->itr |= 0xf0000000L;
        monitor->itr /= SCALE_I_MADC;

        monitor->itip  = ReadRAM(pProHW,pProslic->channel,MADC_ITIP);
        if(monitor->itip & 0x10000000L)
            monitor->itip |= 0xf0000000L;
        monitor->itip /= SCALE_I_MADC;

        monitor->iring  = ReadRAM(pProHW,pProslic->channel,MADC_IRING);
        if(monitor->iring & 0x10000000L)
            monitor->iring |= 0xf0000000L;
        monitor->iring /= SCALE_I_MADC;

        monitor->ilong  = ReadRAM(pProHW,pProslic->channel,MADC_ILONG);
        if(monitor->ilong & 0x10000000L)
            monitor->ilong |= 0xf0000000L;
        monitor->ilong /= SCALE_I_MADC;

    }

    return 0;
}

/*
** Function: Si3226x_AudioGainSetup
**
** Description: 
** Set audio gain of RX and TX paths - presumed that
** all zsynth coefficient presets are 0dB
**
** Presumes that passed impedance preset has already been
** loaded via ProSLIC_ZsythSetup()
*/

int Si3226x_AudioGainSetup(proslicChanType *pProslic, int32 rxgain, int32 txgain, int preset)
{
    Si3226x_dbgSetTXGain(pProslic,txgain,preset,0);
    Si3226x_dbgSetRXGain(pProslic,rxgain,preset,1);
    Si3226x_TXAudioGainSetup(pProslic,0);
    Si3226x_RXAudioGainSetup(pProslic,1);
   
    return 0;
}


/*
** Function: Si3226x_PSTNCheck
**
** Description: 
** Continuous monitoring of longitudinal current.
** If an average of N samples exceed avgThresh or a
** single sample exceeds singleThresh, the linefeed 
** is forced into the open state.
**
** This protects the port from connecting to a live
** pstn line (faster than power alarm).
**
** TODO:  need error handling
*/
int Si3226x_PSTNCheck (proslicChanType *pProslic,proslicPSTNCheckObjType *pPSTNCheck)
{
    uInt8 i;
    /* Adjust buffer index */
    if(pPSTNCheck->count >= pPSTNCheck->samples)
    {
        pPSTNCheck->buffFull = TRUE;
        pPSTNCheck->count = 0;   /* reset buffer ptr */
    }

    /* Read next sample */
    pPSTNCheck->ilong[pPSTNCheck->count]  = ReadRAM(pProHW,pProslic->channel,MADC_ILONG);
    if(pPSTNCheck->ilong[pPSTNCheck->count] & 0x10000000L)
        pPSTNCheck->ilong[pPSTNCheck->count] |= 0xf0000000L;
    pPSTNCheck->ilong[pPSTNCheck->count] /= SCALE_I_MADC;

    /* Monitor magnitude only */
    if(pPSTNCheck->ilong[pPSTNCheck->count] < 0)
        pPSTNCheck->ilong[pPSTNCheck->count] = -pPSTNCheck->ilong[pPSTNCheck->count];

    /* Quickly test for single measurement violation */
    if(pPSTNCheck->ilong[pPSTNCheck->count] > pPSTNCheck->singleThresh)
        return 1;  /* fail */

    /* Average once buffer is full */
    if(pPSTNCheck->buffFull == TRUE)  
    {
        pPSTNCheck->avgIlong = 0;
        for(i=0;i<pPSTNCheck->samples; i++)
        {
            pPSTNCheck->avgIlong += pPSTNCheck->ilong[i];
        }
        pPSTNCheck->avgIlong /= pPSTNCheck->samples;

        if(pPSTNCheck->avgIlong > pPSTNCheck->avgThresh)    
        {
            /* reinit obj and return fail */
            pPSTNCheck->count = 0;
            pPSTNCheck->buffFull = FALSE;
            return 1;
        }
        else
        {
            pPSTNCheck->count++;
            return 0;
        }   
    }
    else
    {
        pPSTNCheck->count++;
        return 0;
    }
}

/*
** Function: Si3226x_SetPwrsaveMode
**
** Description: 
** Enable or disable powersave mode
**
** Returns:
** RC_NONE
*/
int Si3226x_SetPowersaveMode (proslicChanType *pProslic, int pwrsave)
{
uInt8 regData;

    if(pProslic->channelType != PROSLIC) {
        return RC_NONE;    /* Ignore DAA channels */
    }

    regData = ReadReg(pProHW,pProslic->channel, ENHANCE);

    if(pwrsave == PWRSAVE_DISABLE)  
    {
        regData &= 0x07;
    }
    else
    {
        regData |= 0x30;
    }

    WriteReg(pProHW,pProslic->channel, ENHANCE, regData);

    return RC_NONE;
}



/*
** Function: delay_poll
**
** Description: 
** Delay function called within PSTN detection functions
**
** Return Value:
** none
*/
#ifdef PSTN_DET_ENABLE
static void Si3226x_polled_delay(proslicTestStateType *pState, unsigned short delay)
{
unsigned short delayCount;

    if((delay/PSTN_DET_POLL_RATE) < 2)
        delayCount = 0;
    else
        delayCount = (delay/PSTN_DET_POLL_RATE) - 2;

    pState->waitIterations++;
    if((pState->waitIterations == delayCount) || (delayCount == 0))
    {
        pState->waitIterations = 0;
        pState->stage++;
    }
}
#endif


/*
** Function: Si3226x_GetRAMScale
**
** Description: 
** Read scale factor for passed RAM location
**
** Return Value:
** int32 scale
*/
static int32 Si3226x_GetRAMScale(uInt16 addr)
{
int32 scale;

    switch(addr)
    {
        case MADC_ILOOP:
        case MADC_ITIP:
        case MADC_IRING:
        case MADC_ILONG:
            scale = SCALE_I_MADC;
        break;

        case MADC_VTIPC:
        case MADC_VRINGC:
        case MADC_VBAT:
        case MADC_VLONG:
        case VDIFF_SENSE:
        case VDIFF_FILT:
        case VDIFF_COARSE:
        case VTIP:
        case VRING:
            scale = SCALE_V_MADC;
        break;

        default:
            scale = 1;
        break;
    }

    return scale;
}

/*
** Function: Si3226x_ReadMADCScaled
**
** Description: 
** Read MADC (or other sensed voltages/currents) and
** return scaled value in int32 format.
**
** Return Value:
** int32 voltage in mV or
** int32 current in uA
*/
int32 Si3226x_ReadMADCScaled(proslicChanType_ptr pProslic,uInt16 addr, int32 scale)
{
int32 data;

    /* 
    ** Read 29-bit RAM and sign extend to 32-bits
    */
    data = ReadRAM(pProHW,pProslic->channel,addr);
    if(data & 0x10000000L)
        data |= 0xF0000000L;

    /*
    ** Scale to provided value, or use defaults if scale = 0
    */
    if(scale == 0)
        scale = Si3226x_GetRAMScale(addr);

    data /= scale;

    return data;
}

#ifdef PSTN_DET_ENABLE
/*
** Function: abs_int32
**
** Description: 
** abs implementation for int32 type
*/
static int32 abs_int32(int32 a)
{
    if(a < 0)
        return -1*a;
    return a;
}

/*
** Function: Si3226x_DiffPSTNCheck
**
** Description: 
** Monitor for excessive longitudinal current, which
** would be present if a live pstn line was connected
** to the port.
**
** Returns:
** RC_NONE             - test in progress
** RC_COMPLETE_NO_ERR  - test complete, no alarms or errors
** RC_PSTN_OPEN_FEMF   - test detected foreign voltage
** 
*/

int Si3226x_DiffPSTNCheck (proslicChanType *pProslic, proslicDiffPSTNCheckObjType *pPSTNCheck){
    uInt8 loop_status;
    int i;

    if(pProslic->channelType != PROSLIC) {
        return RC_CHANNEL_TYPE_ERR;    /* Ignore DAA channels */
    }


    switch(pPSTNCheck->pState.stage) 
    {
        case 0: 
            /* Optional OPEN foreign voltage measurement - only execute if LCS = 0 */
            /* Disable low power mode */
            pPSTNCheck->enhanceRegSave = ReadReg(pProHW,pProslic->channel,ENHANCE);
            if(pProslic->deviceId->chipRev != 0) {  /* must stay in pwrsave mode on rev A */
                WriteReg(pProHW,pProslic->channel, ENHANCE, pPSTNCheck->enhanceRegSave&0x07); /* Disable powersave */
            }
            pPSTNCheck->vdiff1_avg = 0;
            pPSTNCheck->vdiff2_avg = 0;
            pPSTNCheck->iloop1_avg = 0;
            pPSTNCheck->iloop2_avg = 0;
            pPSTNCheck->return_status = RC_COMPLETE_NO_ERR;
            /* Do OPEN state hazardous voltage measurement if enabled and ONHOOK */
            Si3226x_ReadHookStatus(pProslic,&loop_status);
            if((loop_status == ONHOOK)&&(pPSTNCheck->femf_enable == 1)) 
                pPSTNCheck->pState.stage++;
            else
                pPSTNCheck->pState.stage = 10;
            return RC_NONE;

        case 1:
            /* Change linefeed to OPEN state for HAZV measurement, setup coarse sensors */
            pPSTNCheck->lfstate_entry = ReadReg(pProHW,pProslic->channel, LINEFEED);
            ProSLIC_SetLinefeedStatus(pProslic,LF_OPEN);
            pPSTNCheck->pState.stage++;
            return RC_NONE;

        case 2:
            /* Settle */
            Si3226x_polled_delay(&(pPSTNCheck->pState), PSTN_DET_OPEN_FEMF_SETTLE);
            return RC_NONE;

        case 3: 
            /* Measure HAZV */
            pPSTNCheck->vdiff_open = Si3226x_ReadMADCScaled(pProslic,VDIFF_COARSE,0);
            /* Stop PSTN check if differential voltage > max_femf_vopen present */
#ifdef ENABLE_DEBUG
            if (pProslic->debugMode)
            {
                LOGPRINT("Si3226x Diff PSTN : Vopen = %d mV\n", pPSTNCheck->vdiff_open);
            }
#endif
            if(abs_int32(pPSTNCheck->vdiff_open) > pPSTNCheck->max_femf_vopen)
            {
                pPSTNCheck->pState.stage = 70;
                pPSTNCheck->return_status = RC_PSTN_OPEN_FEMF;
            }
            else
            {
                pPSTNCheck->pState.stage = 10;
            }
            return 0;

        case 10:  
            /* Load first DC feed preset */
            ProSLIC_DCFeedSetup(pProslic,pPSTNCheck->dcfPreset1);
            ProSLIC_SetLinefeedStatus(pProslic,LF_FWD_ACTIVE);
            pPSTNCheck->pState.stage++;
            return RC_NONE; 

        case 11:
            /* Settle */
            Si3226x_polled_delay(&(pPSTNCheck->pState), PSTN_DET_DIFF_IV1_SETTLE);
            return RC_NONE;

        case 12:
            /* Measure VDIFF and ILOOP, switch to 2nd DCFEED setup */
            pPSTNCheck->vdiff1[pPSTNCheck->pState.sampleIterations] = Si3226x_ReadMADCScaled(pProslic,VDIFF_FILT,0);
            pPSTNCheck->iloop1[pPSTNCheck->pState.sampleIterations] = Si3226x_ReadMADCScaled(pProslic,MADC_ILOOP,0);
#ifdef ENABLE_DEBUG
            if (pProslic->debugMode)
            {
                LOGPRINT("Si3226x Diff PSTN : Vdiff1[%d] = %d mV\n", pPSTNCheck->pState.sampleIterations,pPSTNCheck->vdiff1[pPSTNCheck->pState.sampleIterations]);
                LOGPRINT("Si3226x Diff PSTN : Iloop1[%d] = %d uA\n", pPSTNCheck->pState.sampleIterations,pPSTNCheck->iloop1[pPSTNCheck->pState.sampleIterations]);
            }
#endif
            pPSTNCheck->pState.sampleIterations++;
            if(pPSTNCheck->pState.sampleIterations >= pPSTNCheck->samples)
            {
                ProSLIC_DCFeedSetup(pProslic,pPSTNCheck->dcfPreset2);
                pPSTNCheck->pState.stage++;
                pPSTNCheck->pState.sampleIterations = 0;
            }
            return RC_NONE;

        case 13:
            /* Settle feed 500ms */
            Si3226x_polled_delay(&(pPSTNCheck->pState), PSTN_DET_DIFF_IV2_SETTLE);
            return RC_NONE;

        case 14:
            /* Measure VDIFF and ILOOP*/
            pPSTNCheck->vdiff2[pPSTNCheck->pState.sampleIterations] = Si3226x_ReadMADCScaled(pProslic,VDIFF_FILT,0);
            pPSTNCheck->iloop2[pPSTNCheck->pState.sampleIterations] = Si3226x_ReadMADCScaled(pProslic,MADC_ILOOP,0);
#ifdef ENABLE_DEBUG
            if (pProslic->debugMode)
            {
                LOGPRINT("Si3226x Diff PSTN : Vdiff2[%d] = %d mV\n", pPSTNCheck->pState.sampleIterations,pPSTNCheck->vdiff2[pPSTNCheck->pState.sampleIterations]);
                LOGPRINT("Si3226x Diff PSTN : Iloop2[%d] = %d uA\n", pPSTNCheck->pState.sampleIterations,pPSTNCheck->iloop2[pPSTNCheck->pState.sampleIterations]);
            }
#endif
            pPSTNCheck->pState.sampleIterations++;
            if(pPSTNCheck->pState.sampleIterations >= pPSTNCheck->samples)
            {
                /* Compute averages */
                for (i=0; i<pPSTNCheck->samples; i++)
                {
                    pPSTNCheck->vdiff1_avg += pPSTNCheck->vdiff1[i];
                    pPSTNCheck->iloop1_avg += pPSTNCheck->iloop1[i];
                    pPSTNCheck->vdiff2_avg += pPSTNCheck->vdiff2[i];
                    pPSTNCheck->iloop2_avg += pPSTNCheck->iloop2[i];
                }
                pPSTNCheck->vdiff1_avg /= pPSTNCheck->samples;
                pPSTNCheck->iloop1_avg /= pPSTNCheck->samples;
                pPSTNCheck->vdiff2_avg /= pPSTNCheck->samples;
                pPSTNCheck->iloop2_avg /= pPSTNCheck->samples;               
                
                /* Force small (probably offset) currents to minimum value */
                if(abs_int32(pPSTNCheck->iloop1_avg) < PSTN_DET_MIN_ILOOP) pPSTNCheck->iloop1_avg = PSTN_DET_MIN_ILOOP;
                if(abs_int32(pPSTNCheck->iloop2_avg) < PSTN_DET_MIN_ILOOP) pPSTNCheck->iloop2_avg = PSTN_DET_MIN_ILOOP;                


                /* Calculate measured loop impedance */          
                pPSTNCheck->rl1 = abs_int32((pPSTNCheck->vdiff1_avg*1000L)/pPSTNCheck->iloop1_avg);
                pPSTNCheck->rl2 = abs_int32((pPSTNCheck->vdiff2_avg*1000L)/pPSTNCheck->iloop2_avg);
                
                /* Force non-zero loop resistance */
                if(pPSTNCheck->rl1 == 0) pPSTNCheck->rl1 = 1;
                if(pPSTNCheck->rl2 == 0) pPSTNCheck->rl2 = 1;

                /* Qualify loop impedances */
                pPSTNCheck->rl_ratio = (pPSTNCheck->rl1*1000L)/pPSTNCheck->rl2;
#ifdef ENABLE_DEBUG
            if (pProslic->debugMode)
            {
                LOGPRINT("Si3226x DiffPSTN :  VDIFF1  =  %d mV\n",pPSTNCheck->vdiff1_avg);
                LOGPRINT("Si3226x DiffPSTN :  ILOOP1  =  %d uA\n",pPSTNCheck->iloop1_avg);
                LOGPRINT("Si3226x DiffPSTN :  VDIFF2  =  %d mV\n",pPSTNCheck->vdiff2_avg);
                LOGPRINT("Si3226x DiffPSTN :  ILOOP2  =  %d uA\n",pPSTNCheck->iloop2_avg);
                LOGPRINT("Si3226x DiffPSTN :  RL1  =  %d ohm\n",pPSTNCheck->rl1);
                LOGPRINT("Si3226x DiffPSTN :  RL2  =  %d ohm\n",pPSTNCheck->rl2);
                LOGPRINT("Si3226x DiffPSTN :  RL_Ratio  =  %d \n",pPSTNCheck->rl_ratio);            
            }
#endif
           
                /* Restore */
                pPSTNCheck->pState.sampleIterations = 0; 
                pPSTNCheck->pState.stage = 70;
            }
            return RC_NONE;
    
        case 70:  /* Reset test state, restore entry conditions */
            ProSLIC_DCFeedSetup(pProslic,pPSTNCheck->entryDCFeedPreset);
            ProSLIC_SetLinefeedStatus(pProslic,pPSTNCheck->lfstate_entry);
            if(pProslic->deviceId->chipRev != 0) { 
                WriteReg(pProHW,pProslic->channel,ENHANCE, pPSTNCheck->enhanceRegSave);
            }
            pPSTNCheck->pState.stage = 0;
            pPSTNCheck->pState.waitIterations = 0;
            pPSTNCheck->pState.sampleIterations = 0;
            return pPSTNCheck->return_status;
            
    }
return RC_NONE;
}

#endif

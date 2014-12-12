/** \file vp580_api.h
 * vp580_api.h
 *
 *  Header file that define the Vp580 Device and Line Objects
 *
 * Copyright (c) 2010, Zarlink Semiconductor, Inc.
 *
 * $Revision: 6419 $
 * $LastChangedDate: 2010-02-12 16:40:10 -0600 (Fri, 12 Feb 2010) $
 */

#ifndef VP580_API_H
#define VP580_API_H

#include "vp_hal.h"
#include "vp_CSLAC_types.h"

#define VP580_MAX_MPI_DATA  15   /* Max data from any MPI read command */

#ifndef VP580_UL_SIGREG_LEN
#define VP580_UL_SIGREG_LEN         0x01
#endif

#define VP580_INT_SEQ_LEN       22
#define VP580_MAX_NUM_STATES  10

/**< Required Vp580 Device and Line Objects for user instantiation if a Vp580
 * device is used
 */

/**< Structure that defines the Vp580 Device Profile. Current as of the first
 * Device Profile version (ver 0).
 */
typedef struct {
    uint8  maxNumInterrupts;
    uint16 pcmClkRate;      /**< Used to verify valid TX/RX Timeslot setting */
    uint8  mClkMask;
    uint16 tickRate;        /**< Primary API-II tick for this device */
    uint8  devCfg1;
    uint8  debounceReg;
    uint8  clockSlot;
} Vp580DeviceProfileType;

/**< Line Status types to minimize code space in line object (compared to each
 * status being maintined by a uint8 type)
 */
typedef enum {
    VP580_INIT_STATUS = 0x0000,

    VP580_IS_FXO = 0x0001,  /**< Set if the line is configured for FXO */

    VP580_SLS_CALL_FROM_API = 0x0002    /**< Set if Set Line State is called
                                         * from an API function (e.g., cadence).
                                         */
} Vp580LineStatusType;

typedef struct {
    uint8 channelId;    /**< Channel # for "this" line on the device.  Indexed
                         * starting at 0, should not exceed the max number of
                         * lines supported by the device - 1 (max = 2, then
                         * channelId = {0, 1}
                         */

    VpTermType termType;    /**< Termination type */

    Vp580LineStatusType status; /**< Keeps track of several line state/config */

    VpSeqDataType cadence;      /**< Sequencer related information */

    VpDialPulseDetectType dpStruct; /**< Used on FXS lines for detecting pulse
                                     * digits
                                     */

    VpDigitGenerationDataType digitGenStruct;   /**< Used on FXO lines for
                                                 * generating pulse digits
                                                 */

    VpOptionCodecType codec;    /**< Codec mode used on this line. Maintained in
                                 * the API-II to reduce MPI activity which is a
                                 * shared resource and requires CRITICAL code
                                 * section handling.
                                 */

    VpOptionEventMaskType lineEventsMask;
    VpOptionEventMaskType lineEvents;

    VpOptionPulseModeType pulseMode;

    uint8 signalingData;    /**< Holds data for Signaling events on this line */
    uint8 processData;      /**< Holds data for Process events on this line */
    uint16 fxoData;         /**< Timestamp of event */

    VpCslacTimerStruct lineTimers; /**< Timers for "this" line */

    VpApiIntLineStateType lineState;    /**< Line state info used for state
                                         * transition handling and recall
                                         */
#ifdef CSLAC_SEQ_EN
    /**< Array to control internally run sequences */
    VpProfileDataType intSequence[VP580_INT_SEQ_LEN];
#endif

    /* Items saved from the Ring Profile: */
    struct {
        int method;
        uint16 timerMs;
        int state;
    } ringParams;

    VpProfilePtrType pRingingCadence;   /**< Currently used ringing cadence on
                                         * this line
                                         */

    uint16 lineEventHandle; /**< Line specific event handle information */

    VpOptionRingControlType ringCtrl;

    VpOptionPcmTxRxCntrlType pcmTxRxCtrl;   /* Defines how the PCM highway is
                                             * set for "talk" linestates
                                             */

    struct {
        uint16 gxInt;       /**< Cached GX register, in 2.14 int format */
        uint16 grInt;       /**< Cached GR register, in 2.14 int format */
    } gain;

    uint8 lineStateBytes[VP580_MAX_NUM_STATES]; /**< I/O Data bytes to set a
                                                 * line state
                                                 */
    uint16 lineStateExist;  /* Bit map of the line states that are defined */

    bool lineStateInit;         /**< TRUE if line state map is required and
                                 * initialized for this line
                                 */

    uint8 detMap;               /**< Map in I/O data for detect bit */
    uint8 bitMask;      /**< Bits that are used by termination type */

    /* This registers may be used for detector status */
    uint8 ioDataReg;    /* I/O Data Register for this channel */
    uint8 ioDirReg;     /* I/O Direction Register for this channel */
    bool ringingDet;    /**< TRUE when ringing is detected on FXO line */

    uint8 ringDetMin;   /**< Minimum ringing detect period (FXO line) in 500uS
                         * increments
                         */

    uint8 ringDetMax;   /**< Maximum ringing detect period (FXO line) in 500uS
                         * increments
                         */

    VpLineIdType lineId;	/**< Application provided value for mapping a line to
                             * a line context
                             */

    /* For runtime enabling of debug output: */
    uint32 debugSelectMask;

} Vp580LineObjectType;

typedef enum {
    VP580_RSVD = 0x0001,            /* Not Used */
    VP580_FORCE_SIG_READ = 0x0002   /* Set to force a signaling register read */
} Vp580DeviceStateIntType;

typedef struct {
    VpDeviceIdType deviceId;    /**< Device chip select ID defined by user */
    VpDeviceStaticInfoType staticInfo;  /**< Info that will not change during
                                         * runtime
                                         */
    VpDeviceDynamicInfoType dynamicInfo;    /**< Info that will change during
                                             * runtime
                                             */

    /**< State of signaling interrupt register */
    uint8 intReg[VP580_UL_SIGREG_LEN];

    VpCSLACDeviceStatusType status;
    Vp580DeviceStateIntType stateInt;

    VpOptionEventMaskType deviceEventsMask;
    VpOptionEventMaskType deviceEvents;

    VpOptionPulseType pulseSpecs;
    VpOptionPulseType pulseSpecs2;

    uint16 devTimer[VP_DEV_TIMER_LAST];
    Vp580DeviceProfileType devProfileData;
    VpCSLACDeviceProfileTableType devProfileTable;
    VpCSLACProfileTableEntryType profEntry;

    /**< State of signaling interrupt register - length 2 bytes */
    uint8 mpiData[VP580_MAX_MPI_DATA];  /**< Buffer for MPI Low level reads to
                                         * hold maximum amount of MPI data that
                                         * is possible
                                         */

    uint8 mpiLen;       /**< Length of data to be copied into mpiData buffer */

    uint16 eventHandle;  /** Application defined event handle */
    uint16 timeStamp;   /**< Used to track event timing. Increment by ticks */

    VpGetResultsOptionsType getResultsOption;
    VpRelGainResultsType relGainResults;

    uint8 rtdReg;       /* Real-Time Data Register for this device */
    uint8 clkFailReg;   /* Register containing clock fault status */

    /* For runtime enabling of debug output: */
    uint32 debugSelectMask;

} Vp580DeviceObjectType;

#endif  /**< vp580_api.h */





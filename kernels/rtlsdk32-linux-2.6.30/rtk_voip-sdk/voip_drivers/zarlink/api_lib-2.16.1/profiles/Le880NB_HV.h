/*
 * Le880NB_HV.h --
 *
 * This header file exports the Profile data types
 *
 * Project Info --
 *   File:   V:\work\VoIP\branches\VoIP-jungle-2.4-1.5\rlxlinux-v2.2\linux-2.6.30\rtk_voip-1.5\voip_drivers\zarlink\api_lib-2.16.1\profiles\Le880NB_HV.vpw
 *   Type:   Le71HR0864G Line Module Project - VBL Reg ABS Power Supply and 8.192MHz PCLK
 *   Date:   Thursday, February 17, 2011 18:53:42
 *   Device: VoicePort Le88286
 *
 *   This file was generated with Profile Wizard Version: P2.0.0
 */

#ifndef LE880NB_HV_H
#define LE880NB_HV_H

#ifdef VP_API_TYPES_H
#include "vp_api_types.h"
#else
typedef unsigned char VpProfileDataType;
#endif


/************** Device Parameters **************/
extern const VpProfileDataType LE880_ABS_VBL_FLYBACK[];/* Device Configuration Data */

/************** AC Filter Coefficients **************/
extern const VpProfileDataType LE880_AC_FXS_RF14_DEF[];/* AC FXS RF14 600 Ohm Coefficients [Default} */
extern const VpProfileDataType LE880_AC_FXS_RF14_900[];/* AC FXS RF14 900 Ohm Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_AU[];/* AC FXS RF14 Australia 220+820//220nF Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_AT[];/* AC FXS RF14 Austria 220+820//115nF Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_BE[];/* AC FXS RF14 Belgium 150+830//72 Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_BR[];/* AC FXS RF14 Brazil 900 Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_CN[];/* AC FXA RF14 China 200+680//100nF Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_CZ[];/* AC FXS RF14 Czech Republic 600 Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_DK[];/* AC FXS RF14 Denmark 300+1000//220nF Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_EU[];/* AC FXS RF14 ETSI 270+750//150nF Harmonized Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_FI[];/* AC FXS RF14 Finland 270+910//120 Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_FR[];/* AC FXS RF14 France 215+1000//137nF Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_DE[];/* AC FXS RF14 German 220+820//115nF Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_GR[];/* AC FXS RF14 Greece 400+500//50nF Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_HU[];/* AC FXS RF14 Hungary 600 Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_IN[];/* AC FXS RF14 India 600 Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_IT[];/* AC FXS RF14 Italy 180+630//60nF Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_JP[];/* AC FXS RF14 Japan 600+1uF Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_KR[];/* AC FXS RF14 S. Korea 600 Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_MX[];/* AC FXS RF14 Mexico 600 Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_NL[];/* AC FXS RF14 Netherlands 600/340+422//100 Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_NZ[];/* AC FXS RF14 New Zealand 370+620//310nF Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_NO[];/* AC FXS RF14 Norway 120+820//110nF Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_PT[];/* AC FXS RF14 Portugal 600 Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_SI[];/* AC FXS RF14 Slovenia 600/220+820//115 Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_ES[];/* AC FXS RF14 Spain 220+820//120nF Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_SE[];/* AC FXS RF14 Sweden 200+1000//100nF_900//30nF Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_GB[];/* AC FXS RF14 U.K. 300+1000//220_370+620//310nF Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_US_loaded[];/* AC FXS RF14 US 900//2.16uF_1650//(100+5nF) Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_US_Nonloaded[];/* AC FXS RF14 US 900//2,16uF_800//(100+50nF) Coefficients */
extern const VpProfileDataType LE880_AC_FXS_RF14_US_SS[];/* AC FXS RF14 US 900//2.16uF Coefficients */

/************** DC Feed Parameters **************/
extern const VpProfileDataType LE880_DC_FXS_DEF[];   /* DC FXS Default 22 mA current feed */

/************** Ring Signal Parameters **************/
extern const VpProfileDataType LE880_RING_DEF[];     /* Default Rnging, 25Hz, Sinewave, 80Vpk */
extern const VpProfileDataType LE880_RING_US[];      /* US Ringing, 20Hz, Sinewave, 80Vpk */
extern const VpProfileDataType LE880_RING_CA[];      /* Canada Ringing 20Hz, Sinewave, 80Vpk */
extern const VpProfileDataType LE880_RING_FR[];      /* France Ringing 50Hz, Sinewave, 80Vpk */
extern const VpProfileDataType LE880_RING_JP[];      /* Japan Ringing 16Hz, Sinewave, 80Vpk */
extern const VpProfileDataType LE880_RING_KR[];      /* S. Korea Ringing 20Hz, Sinewave, 80Vpk */
extern const VpProfileDataType LE880_RING_TW[];      /* Taiwan Ringing 20Hz, Sinewave, 80Vpk */
extern const VpProfileDataType LE880_RING_HK[];      /* Hong Kong Ringing 20Hz, Sinewave, 80Vpk */
extern const VpProfileDataType LE880_RING_SG[];      /* Singapore Ringing, 24Hz, Sinewave, 80Vpk */
extern const VpProfileDataType LE880_RING_AT[];      /* Austria Ringing 50Hz, Sinewave, 80Vpk */
extern const VpProfileDataType LE880_RING_AU[];      /* Australia Ringing 20Hz, Sinewave, 80Vpk */

/************** Call Progress Tones **************/

/************** Cadence Definitions **************/
extern const VpProfileDataType LE880_RING_CAD_STD[]; /* Short Ringing Cadence (2 sec on 4 sec off) */
extern const VpProfileDataType LE880_RING_CAD_SHORT[];/* Short Ringing Cadence (0.5 sec on 0.5 sec off) */

/************** Caller ID **************/

/************** Metering Profile **************/

#endif /* LE880NB_HV_H */


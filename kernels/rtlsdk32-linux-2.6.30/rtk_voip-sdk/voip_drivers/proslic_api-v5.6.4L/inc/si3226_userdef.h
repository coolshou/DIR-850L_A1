#ifndef SI3226_USERDEF_H
#define SI3226_USERDEF_H

void Si3226_Set_Ring_Cadence_ON(proslicChanType *pProslic, unsigned short msec);

void Si3226_Set_Ring_Cadence_OFF(proslicChanType *pProslic, unsigned short msec);

void Si3226_SendNTTCAR(proslicChanType *pProslic);

unsigned int Si3226_SendNTTCAR_check(unsigned int chid, proslicChanType *pProslic, unsigned long time_out);

void Si3226_Set_Impendance_Silicon(proslicChanType *pProslic, unsigned short country, unsigned short impd /*reserve*/);

void Si3226_Set_Impendance(proslicChanType *pProslic, unsigned short preset);

int Si3226_GetLinefeedStatus (proslicChanType *pProslic,uInt8 *newLinefeed);

int SI3226_SetUserMode(proslicChanType *pProslic, int on);

#endif

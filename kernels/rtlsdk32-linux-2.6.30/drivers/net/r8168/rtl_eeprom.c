/*
################################################################################
# 
# r8101 is the Linux device driver released for RealTek RTL8101E, RTL8102E,
# and RTL8103E Fast Ethernet controllers with PCI-Express interface.
# 
# Copyright(c) 2009 Realtek Semiconductor Corp. All rights reserved.
# 
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
# 
# You should have received a copy of the GNU General Public License along with
# this program; if not, see <http://www.gnu.org/licenses/>.
# 
# Author:
# Realtek NIC software team <nicfae@realtek.com>
# No. 2, Innovation Road II, Hsinchu Science Park, Hsinchu 300, Taiwan
# 
################################################################################
*/

/*
 * This product is covered by one or more of the following patents:
 * US5,307,459, US5,434,872, US5,732,094, US6,570,884, US6,115,776, and US6,327,625.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/ethtool.h>
#include <linux/netdevice.h>
#include <linux/delay.h>

#include <asm/io.h>

#include "r8168.h"
#include "rtl_eeprom.h"

//-------------------------------------------------------------------
//rtl_eeprom_type():
//	tell the eeprom type
//return value:
//	0: the eeprom type is 93C46
//	1: the eeprom type is 93C56 or 93C66
//-------------------------------------------------------------------
int rtl_eeprom_type(void __iomem *ioaddr)
{
	return RTL_R32(RxConfig) & RxCfg_9356SEL;
}

void rtl_eeprom_cleanup(void __iomem *ioaddr)
{
	u8 x;

	x = RTL_R8(Cfg9346);
	x &= ~(Cfg9346_EEDI | Cfg9346_EECS);

	RTL_W8(Cfg9346, x);

	rtl_raise_clock(&x, ioaddr);
	rtl_lower_clock(&x, ioaddr);
}

int rtl_eeprom_cmd_done(void __iomem *ioaddr)
{
	u8 x;
	int i;

	rtl_stand_by(ioaddr);

	for (i = 0; i < 50000; i++) {
		x = RTL_R8(Cfg9346);

		if (x & Cfg9346_EEDO) {
			udelay(RTL_CLOCK_RATE * 2 * 3);
			return 0;	
		}
		udelay(1);
	}

	return -1;
}

//-------------------------------------------------------------------
//rtl_eeprom_read_sc():
//	read one word from eeprom
//-------------------------------------------------------------------
u16 rtl_eeprom_read_sc(void __iomem *ioaddr, u16 reg)
{

	int addr_sz = 6;
	u8 x;
	u16 data;

	if (rtl_eeprom_type(ioaddr))
		addr_sz = 8;
	else
		addr_sz = 6;

	x = RTL_R8(Cfg9346);
	x &= ~(Cfg9346_EEDI | Cfg9346_EEDO | Cfg9346_EESK);
	x |= Cfg9346_EEM1 | Cfg9346_EECS;
	RTL_W8(Cfg9346, x);

	rtl_shift_out_bits(RTL_EEPROM_READ_OPCODE, 3, ioaddr);
	rtl_shift_out_bits(reg, addr_sz, ioaddr);

	data = rtl_shift_in_bits(ioaddr);

	rtl_eeprom_cleanup(ioaddr);

	return data;
}

//-------------------------------------------------------------------
//rtl_eeprom_write_sc():
//	write one word to a specific address in the eeprom
//-------------------------------------------------------------------
void rtl_eeprom_write_sc(void __iomem *ioaddr, u16 reg, u16 data)
{
	u8 x;
	int addr_sz = 6;
	int w_dummy_addr = 4;

	if (rtl_eeprom_type(ioaddr)) {
		addr_sz = 8;
		w_dummy_addr = 6;
	} else {
		addr_sz = 6;
		w_dummy_addr = 4;
	}

	x = RTL_R8(Cfg9346);
	x &= ~(Cfg9346_EEDI | Cfg9346_EEDO | Cfg9346_EESK);
	x |= Cfg9346_EEM1 | Cfg9346_EECS;
	RTL_W8(Cfg9346, x);

	rtl_shift_out_bits(RTL_EEPROM_EWEN_OPCODE, 5, ioaddr);
	rtl_shift_out_bits(reg, w_dummy_addr, ioaddr);
	rtl_stand_by(ioaddr);

	rtl_shift_out_bits(RTL_EEPROM_ERASE_OPCODE, 3, ioaddr);
	rtl_shift_out_bits(reg, addr_sz, ioaddr);
	if (rtl_eeprom_cmd_done(ioaddr) < 0) {
		return;
	}
	rtl_stand_by(ioaddr);

	rtl_shift_out_bits(RTL_EEPROM_WRITE_OPCODE, 3, ioaddr);
	rtl_shift_out_bits(reg, addr_sz, ioaddr);
	rtl_shift_out_bits(data, 16, ioaddr);
	if (rtl_eeprom_cmd_done(ioaddr) < 0) {
		return;
	}
	rtl_stand_by(ioaddr);

	rtl_shift_out_bits(RTL_EEPROM_EWDS_OPCODE, 5, ioaddr);
	rtl_shift_out_bits(reg, w_dummy_addr, ioaddr);

	rtl_eeprom_cleanup(ioaddr);
}

void rtl_raise_clock(u8 *x, void __iomem *ioaddr)
{

	*x = *x | Cfg9346_EESK;
	RTL_W8(Cfg9346, *x);
	udelay(RTL_CLOCK_RATE);
}

void rtl_lower_clock(u8 *x, void __iomem *ioaddr)
{

	*x = *x & ~Cfg9346_EESK;
	RTL_W8(Cfg9346, *x);
	udelay(RTL_CLOCK_RATE);
}

void rtl_shift_out_bits(int data, int count, void __iomem *ioaddr)
{
	u8 x;
	int  mask;

	mask = 0x01 << (count - 1);
	x = RTL_R8(Cfg9346);
	x &= ~(Cfg9346_EEDI | Cfg9346_EEDO);

	do {
		x &= ~Cfg9346_EEDI;
		if (data & mask)
			x |= Cfg9346_EEDI;

		RTL_W8(Cfg9346, x);
		udelay(RTL_CLOCK_RATE);
		rtl_raise_clock(&x, ioaddr);
		rtl_lower_clock(&x, ioaddr);
		mask = mask >> 1;
	} while(mask);

	x &= ~Cfg9346_EEDI;
	RTL_W8(Cfg9346, x);
}

u16 rtl_shift_in_bits(void __iomem *ioaddr)
{
	u8 x;
	u16 d, i;

	x = RTL_R8(Cfg9346);
	x &= ~(Cfg9346_EEDI | Cfg9346_EEDO);

	d = 0;

	for (i = 0; i < 16; i++) {
		d = d << 1;
		rtl_raise_clock(&x, ioaddr);

		x = RTL_R8(Cfg9346);
		x &= ~Cfg9346_EEDI;

		if (x & Cfg9346_EEDO)
			d |= 1;

		rtl_lower_clock(&x, ioaddr);
	}

	return d;
}

void rtl_stand_by(void __iomem *ioaddr)
{
	u8 x;

	x = RTL_R8(Cfg9346);
	x &= ~(Cfg9346_EECS | Cfg9346_EESK);
	RTL_W8(Cfg9346, x);
	udelay(RTL_CLOCK_RATE);

	x |= Cfg9346_EECS;
	RTL_W8(Cfg9346, x);
}

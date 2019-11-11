/****************************************************************/
/*                                                              */
/*                          network.c                           */
/*                            DOS-C                             */
/*                                                              */
/*                 Networking Support functions                 */
/*                                                              */
/*                   Copyright (c) 1995, 1999                   */
/*                         James Tabor                          */
/*                      All Rights Reserved                     */
/*                                                              */
/* This file is part of DOS-C.                                  */
/*                                                              */
/* DOS-C is free software; you can redistribute it and/or       */
/* modify it under the terms of the GNU General Public License  */
/* as published by the Free Software Foundation; either version */
/* 2, or (at your option) any later version.                    */
/*                                                              */
/* DOS-C is distributed in the hope that it will be useful, but */
/* WITHOUT ANY WARRANTY; without even the implied warranty of   */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See    */
/* the GNU General Public License for more details.             */
/*                                                              */
/* You should have received a copy of the GNU General Public    */
/* License along with DOS-C; see the file COPYING.  If not,     */
/* write to the Free Software Foundation, 675 Mass Ave,         */
/* Cambridge, MA 02139, USA.                                    */
/****************************************************************/

#include "portab.h"
#include "globals.h"


/* see RBIL D-2152 and D-215D06 before attempting
   to change these two functions!
 */
UWORD get_machine_name(char FAR * netname)
{
  fmemcpy(netname, &net_name, 16);
  return (NetBios);
}

VOID set_machine_name(const char FAR * netname, UWORD name_num)
{
  NetBios = name_num;
  fmemcpy(&net_name, netname, 15);
  net_set_count++;
}

int network_redirector_fp(unsigned cmd, void FAR *s)
{
  return (WORD)network_redirector_mx(cmd, s, 0);
}

int network_redirector(unsigned cmd)
{
  return network_redirector_fp(cmd, NULL);
}

/* Int 2F network redirector functions
 *
 * added by stsp for fdpp project
 */
#define MK_FAR_SCP(x) &(x)

/* seek relative to end of remote file */
UDWORD remote_lseek(void FAR *sft, DWORD new_pos)
{
    iregs regs = {0};
    regs.es = FP_SEG(sft);                  /* ES:DI -> SFT */
    regs.di = FP_OFF(sft);
    regs.d.x = (WORD)(new_pos & 0xffff);    /* CX:DX -> offset in bytes from end */
    regs.c.x = (WORD)(new_pos >> 16);
    regs.a.x = REM_LSEEK;
    call_intr(0x2f, MK_FAR_SCP(regs));
    if (regs.flags & FLG_CARRY)             /* carry set on error, AL=error code */
        return -regs.a.x;
	return MK_ULONG(regs.d.x, regs.a.x);    /* on success, DX:AX -> new file pos */
}

/* get remote file's attributes */
UWORD remote_getfattr(void)
{
    iregs regs = {0};
    regs.a.x = REM_GETATTRZ;
	/* uses SDA for which file, see www.ctyme.com/intr/rb-4320.htm */
    call_intr(0x2f, MK_FAR_SCP(regs));
    if (regs.flags & FLG_CARRY)             /* carry set on error, AL=error code */
        return -regs.a.x;
	/* on succesful return: 
	   AX    = file attributes, 
	   BX:DI = file size,
	   CX    = time stamp of file
	   DX    = date stamp of file
	*/
    return regs.a.x;
}

/* DOS 4+ lock/unlock region */
BYTE remote_lock_unlock(void FAR *sft, BYTE unlock,
    struct remote_lock_unlock FAR *arg)
{
    iregs regs = {0};
    regs.es = FP_SEG(sft);                  /* ES:DI -> sft */
    regs.di = FP_OFF(sft);
    regs.b.b.l = unlock;                    /* BL -> 00h=lock, 01h=unlock */
    regs.c.x = 1;                           /* always 1, # of parameters to lock/unlock */
    regs.ds = FP_SEG(arg);                  /* DS:DX -> parameter array (offset, size)  */
    regs.d.x = FP_OFF(arg);
    regs.a.x = REM_LOCK;
    call_intr(0x2f, MK_FAR_SCP(regs));
    if (regs.flags & FLG_CARRY)             /* carry set on error, no error code */
        return -1;
    return 0;
}

/* qualify remote filename */
BYTE remote_qualify_filename(char FAR *dst, const char FAR *src)
{
    iregs regs = {0};
    regs.es = FP_SEG(dst);                  /* ES:DI -> 128 byte buffer for qualified name */
    regs.di = FP_OFF(dst);
    regs.ds = FP_SEG(src);                  /* DS:SI -> ASCIIZ filename to make canonical  */
    regs.si = FP_OFF(src);
    regs.a.x = REM_FILENAME;
    call_intr(0x2f, MK_FAR_SCP(regs));
    if (regs.flags & FLG_CARRY)             /* carry set on error, no error code */
        return -1;
    return 0;
}

/* get remote disk information */
BYTE remote_getfree(void FAR *cds, void FAR *dst)
{
    UWORD FAR *udst = dst;
    iregs regs = {0};
    regs.es = FP_SEG(cds);                  /* ES:DI -> cds for remote drive */     
    regs.di = FP_OFF(cds);
    regs.a.x = REM_GETSPACE;
    call_intr(0x2f, MK_FAR_SCP(regs));
    if (regs.flags & FLG_CARRY)             /* carry set on error, no error code */
        return -1;
    udst[0] = regs.a.x;                     /* AL -> sectors per cluster, AH -> media byte */
    udst[1] = regs.b.x;                     /* BX -> total clusters     */
    udst[2] = regs.c.x;                     /* CX -> bytes per sector   */
    udst[3] = regs.d.x;                     /* DX -> available clusters */
    return 0;
}

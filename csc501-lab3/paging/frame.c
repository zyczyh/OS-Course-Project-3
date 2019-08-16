/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
	STATWORD ps;
	int i = 0;

	disable(ps);
	for(i = 0; i < NFRAMES; i = i + 1)
	{
		frm_tab[i].fr_status = FRM_UNMAPPED;
		frm_tab[i].fr_pid = -1;
		frm_tab[i].fr_vpno = 0;
		frm_tab[i].fr_refcnt = 0;
		frm_tab[i].fr_type = FR_PAGE;
		frm_tab[i].fr_dirty = 0;
		frm_tab[i].next_fr = -1;
	}
	restore(ps);	
  	return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
	STATWORD ps;
	int i = 0;
	int pid = 0;
	pd_t* pd_entry;
	pt_t* pt_entry;

	disable(ps);
	for(i = 0; i < NFRAMES; i = i + 1)
	{
		if(frm_tab[i].fr_status == FRM_UNMAPPED)
		{
			*avail = i;
			restore(ps);
			return OK;
		}
	}
	if(page_replace_policy == SC)
	{
		while(1)
		{
			pid = frm_tab[SC_curr].fr_pid;
			pd_entry = proctab[pid].pdbr + frm_tab[SC_curr].fr_vpno / 1024 * sizeof(pd_t);
			if(pd_entry->pd_pres != 1)
			{
				restore(ps);
				return SYSERR;
			}
			pt_entry = pd_entry->pd_base * NBPG + frm_tab[SC_curr].fr_vpno % 1024 * sizeof(pt_t);
			if(pt_entry->pt_pres != 1)
			{
				restore(ps);
				return SYSERR;
			}
			if(pt_entry->pt_acc == 1)
			{
				pt_entry->pt_acc = 0;
				if(SC_curr == FIFO_tail)
				{
					SC_curr = FIFO_head;
					SC_prev = -1;
				}
				else
				{
					SC_prev = SC_curr;
					SC_curr = frm_tab[SC_curr].next_fr;
				}
			}
			else
			{
				*avail = SC_curr;
				if(SC_curr == FIFO_head)
				{
					FIFO_head = frm_tab[SC_curr].next_fr;
					frm_tab[SC_curr].next_fr = -1;
					SC_curr = FIFO_head;
					SC_prev = -1;
				}
				else if(SC_curr == FIFO_tail)
				{
					FIFO_tail = SC_prev;
					frm_tab[SC_curr].next_fr = -1;
					SC_curr = FIFO_head;
					SC_prev = -1;
				}
				else
				{
					frm_tab[SC_prev].next_fr = frm_tab[SC_curr].next_fr;
					frm_tab[SC_curr].next_fr = -1;
					SC_curr = frm_tab[SC_prev].next_fr;
				}
				break;
			}
		}
	}
	if(page_replace_policy == FIFO)
	{
		*avail = FIFO_head;
		FIFO_head = frm_tab[FIFO_head].next_fr;
		frm_tab[*avail].next_fr = -1;		
	}
	free_frm(*avail);
  	return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int frm_id)
{
	STATWORD ps;
	int vpno = 0;
	unsigned long long_vaddr = 0;
	pd_t *pde;
	pt_t *pte;
	int bs_id = 0;
	int pageth = 0;
	int pd_offset = 0;
	int pt_offset = 0;
	int pid = 0;

	disable(ps);
	pid = frm_tab[frm_id].fr_pid;
	vpno = frm_tab[frm_id].fr_vpno;
	long_vaddr = vpno * NBPG;
	pd_offset = long_vaddr >> 22;
	pt_offset = (long_vaddr & 0x003FF000) >> 12;
	pde = proctab[pid].pdbr + sizeof(pd_t) * pd_offset;
    pte = pde->pd_base * NBPG + sizeof(pt_t) * pt_offset;
	if(frm_tab[frm_id].fr_type == FR_PAGE)
	{
		pte->pt_pres = 0;
		//invlpg();
		frm_tab[pde->pd_base - FRAME0].fr_refcnt = frm_tab[pde->pd_base - FRAME0].fr_refcnt - 1;
		if(frm_tab[pde->pd_base - FRAME0].fr_refcnt == 0)
		{
			pde->pd_pres = 0;
			frm_tab[pde->pd_base - FRAME0].fr_status = FRM_UNMAPPED;
			frm_tab[pde->pd_base - FRAME0].fr_vpno = 0;
			frm_tab[pde->pd_base - FRAME0].fr_pid = -1;
			frm_tab[pde->pd_base - FRAME0].fr_type = FR_PAGE;
			frm_tab[pde->pd_base - FRAME0].fr_dirty = 0;
			frm_tab[pde->pd_base - FRAME0].next_fr = -1;
		}
		if(bsm_lookup(pid, vpno, &bs_id, &pageth) == SYSERR)
		{
			kill(pid);
			restore(ps);
			return SYSERR;
		}
		if(pte->pt_dirty == 1)
		{
			write_bs(pte->pt_base * NBPG, bs_id, pageth);
		}
	}
	frm_tab[frm_id].fr_status = FRM_UNMAPPED;
	frm_tab[frm_id].fr_pid = -1;
	frm_tab[frm_id].fr_vpno = 0;
	frm_tab[frm_id].fr_refcnt = 0;
	frm_tab[frm_id].fr_type = FR_PAGE;
	frm_tab[frm_id].fr_dirty = 0;
	frm_tab[frm_id].next_fr = -1;
	return OK;
}

SYSCALL map_frm(int frm_id, int pid, int vpno, int type)
{
	frm_tab[frm_id].fr_status = FRM_MAPPED;
	frm_tab[frm_id].fr_pid = pid;
	frm_tab[frm_id].fr_vpno = vpno;
	if(type == FR_TBL)
	{
		frm_tab[frm_id].fr_refcnt = 1;
	}
	else
	{
		frm_tab[frm_id].fr_refcnt = 0;
	}
	frm_tab[frm_id].fr_type = type;
	frm_tab[frm_id].fr_dirty = 0;
	frm_tab[frm_id].next_fr = -1;
	if(type == FR_PAGE && page_replace_policy == SC)
	{
		if(FIFO_head == -1)
		{
			FIFO_head = frm_id;
			FIFO_tail = frm_id;
			SC_curr = FIFO_head;
			SC_prev = -1;
		}
		else
		{
			frm_tab[FIFO_tail].next_fr = frm_id;
			FIFO_tail = frm_id;	
		}
	}
	if(type == FR_PAGE && page_replace_policy == FIFO)
	{
		if(FIFO_head == -1)
		{
			FIFO_head = frm_id;
			FIFO_tail = frm_id;
		}
		else
		{
			frm_tab[FIFO_tail].next_fr = frm_id;
			FIFO_tail = frm_id;	
		}
	}
	return OK;
}


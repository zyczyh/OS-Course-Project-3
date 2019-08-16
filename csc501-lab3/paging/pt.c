/* pt.c - manage the page table and page directory */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/* initialize the global page table */
SYSCALL init_gpt()
{
	STATWORD ps;
	int fr_id = 0;
	int i = 0;
	int j = 0;
	pt_t *pt_entry;

	disable(ps);
	for(i = 0; i < 4; i = i + 1)
	{
		get_frm(&fr_id);
		frm_tab[fr_id].fr_status = FRM_MAPPED;
		frm_tab[fr_id].fr_type = FR_TBL;
		frm_tab[fr_id].fr_pid = NULLPROC;
		frm_tab[fr_id].fr_refcnt = 1024;
		frm_tab[fr_id].fr_dirty = 0;
		frm_tab[fr_id].next_fr = -1;
		frm_tab[fr_id].fr_vpno = -1;
		pt_entry = (FRAME0 + fr_id) * NBPG;
		for(j = 0; j < 1024; j = j + 1)
		{
			pt_entry->pt_pres = 1;
			pt_entry->pt_write = 1;
			pt_entry->pt_user = 0;
			pt_entry->pt_pwt = 0;
			pt_entry->pt_pcd = 0;
			pt_entry->pt_acc = 0;
			pt_entry->pt_dirty = 0;
			pt_entry->pt_mbz = 0;
			pt_entry->pt_global = 1;
			pt_entry->pt_avail = 0;
			pt_entry->pt_base = i*FRAME0+j;	
			pt_entry = pt_entry + 1;			
		}
	}
	restore(ps);
	return OK;
}

/* allocate page directory for each process */
SYSCALL alloc_pd(int pid)
{
	STATWORD ps;
	int fr_id = 0;
	int i = 0;
	int j = 0;
	pd_t *pd_entry;

	disable(ps);
	get_frm(&fr_id);
	proctab[pid].pdbr = (FRAME0 + fr_id) * NBPG;
	frm_tab[fr_id].fr_status = FRM_MAPPED;
	frm_tab[fr_id].fr_type = FR_DIR;
	frm_tab[fr_id].fr_pid = pid;
	frm_tab[fr_id].next_fr = -1;
	frm_tab[fr_id].fr_dirty = 0;
	frm_tab[fr_id].fr_refcnt = 0;
	frm_tab[fr_id].fr_vpno = -1;
	pd_entry = 	(FRAME0 + fr_id) * NBPG;
	for(i = 0; i < 1024; i = i + 1)
	{
		pd_entry->pd_write = 1;
		if(i < 4)
		{
			pd_entry->pd_pres = 1;
			pd_entry->pd_global = 1;
			pd_entry->pd_base = FRAME0 + i;
		}
		else
		{
			pd_entry->pd_pres = 0;
			pd_entry->pd_global = 0;
			pd_entry->pd_base = 0;			
		}
		pd_entry->pd_user = 0;
		pd_entry->pd_pwt = 0;
		pd_entry->pd_pcd = 0;
		pd_entry->pd_acc = 0;
		pd_entry->pd_mbz = 0;
		pd_entry->pd_fmb = 0;
		pd_entry->pd_avail = 0;
		pd_entry = pd_entry + 1;
	}
	restore(ps);
	return OK;
}

/* allocate page table */
SYSCALL alloc_pt(int pid, int* frm_id)
{
	STATWORD ps;
	disable(ps);

	int i = 0;
	pt_t* pt;

	get_frm(frm_id);
	map_frm(*frm_id, pid, -1, FR_TBL);
	pt = (*frm_id + FRAME0) * NBPG;
	for(i = 0; i < 1024; i = i + 1)
	{
		(pt+i)->pt_pres = 0;
		(pt+i)->pt_write = 1;		
		(pt+i)->pt_user = 0;
		(pt+i)->pt_pwt = 0;
		(pt+i)->pt_pcd = 0;
		(pt+i)->pt_acc = 0;
		(pt+i)->pt_dirty = 0;
		(pt+i)->pt_mbz = 0;
		(pt+i)->pt_global = 0;
		(pt+i)->pt_avail = 0;
		(pt+i)->pt_base = 0;		
	}
	restore(ps);
	return OK;
}
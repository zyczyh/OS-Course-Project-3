/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
	STATWORD ps;
	disable(ps);

	unsigned long fault_addr;
	int vpno;
	pd_t* pd;
	pt_t* pt;
	pd_t* pde;
	pt_t* pte;
	int pd_offset;
	int pt_offset;
	int bs_id;
	int pageth;
	int frm_id;

	fault_addr = read_cr2();
	vpno = fault_addr / NBPG;
	//check if it is a legal address
	pde = proctab[currpid].pdbr;
	pd_offset = fault_addr >> 22;
	pt_offset = (fault_addr & 0x003FF000) >> 12;
	pd = pde + pd_offset;
	if(pd->pd_pres == 1)
	{
		frm_tab[pd->pd_base - FRAME0].fr_refcnt = frm_tab[pd->pd_base - FRAME0].fr_refcnt + 1;
	}
	else
	{
		alloc_pt(currpid, &frm_id);
		pd->pd_pres = 1;
		pd->pd_write = 1;
		pd->pd_user = 0;
		pd->pd_pwt = 0;
		pd->pd_pcd = 0;
		pd->pd_acc = 0;
		pd->pd_mbz = 0;
		pd->pd_fmb = 0;
		pd->pd_global = 0;
		pd->pd_avail = 0;
		pd->pd_base = FRAME0 + frm_id;
		frm_tab[pd->pd_base - FRAME0].fr_refcnt = frm_tab[pd->pd_base - FRAME0].fr_refcnt + 1;
	}
	pte = pd->pd_base * NBPG;
	pt = pte + pt_offset;
	if(bsm_lookup(currpid, vpno, &bs_id, &pageth) == SYSERR)
	{
		kill(currpid);
		restore(ps);
		return SYSERR;
	}
	get_frm(&frm_id);
	map_frm(frm_id, currpid, vpno, FR_PAGE);
	read_bs((frm_id + FRAME0) * NBPG, bs_id, pageth);
	pt->pt_pres = 1;
	pt->pt_write = 1;
	pt->pt_user = 0;
	pt->pt_pwt = 0;
	pt->pt_pcd = 0;
	pt->pt_acc = 0;
	pt->pt_dirty = 0;
	pt->pt_mbz = 0;
	pt->pt_global = 0;
	pt->pt_avail = 0;
	pt->pt_base = frm_id + FRAME0;
	write_cr3(pde);
	restore(ps);
	return OK;
}



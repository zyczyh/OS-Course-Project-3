/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
	STATWORD ps;
	int i = 0;

	disable(ps);
	for(i = 0; i < NBS; i = i + 1)
	{
		bsm_tab[i].bs_status = BSM_UNMAPPED;
		bsm_tab[i].bs_pid = -1;
		bsm_tab[i].bs_vpno = 0;
		bsm_tab[i].bs_npages = 0;
	}
	restore(ps);
	return OK;	
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
	STATWORD ps;
	int i = 0;

	disable(ps);
	if(avail == NULL)
	{
		restore(ps);
		return SYSERR;
	}
	for(i = 0; i < NBS; i = i + 1)
	{
		if(bsm_tab[i].bs_status == BSM_UNMAPPED)
		{
			*avail = i;
			restore(ps);
			return OK;
		}
	}
	restore(ps);
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
	STATWORD ps;

	disable(ps);
	bsm_tab[i].bs_status = BSM_UNMAPPED;
	bsm_tab[i].bs_pid = -1;
	bsm_tab[i].bs_vpno = 0;
	bsm_tab[i].bs_npages = 0;
	restore(ps);
	return OK;			
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
	STATWORD ps;

	disable(ps);
	if(proctab[pid].store < 0)
	{
		restore(ps);
		return SYSERR;
	}
	*store = proctab[pid].store;
	*pageth = (vaddr/NBPG) - bsm_tab[*store].bs_vpno;
	restore(ps);
	return OK;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
	STATWORD ps;

	disable(ps);
	proctab[pid].store = source;
	bsm_tab[source].bs_status = BSM_MAPPED;
	bsm_tab[source].bs_pid = pid;
	bsm_tab[source].bs_vpno = vpno;
	bsm_tab[source].bs_npages = npages;
	restore(ps);
	return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	bsd_t source;

	source = proctab[pid].store;
	free_bsm(source);
	proctab[pid].store = -1;
	return OK;		
}



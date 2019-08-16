/* pfintr.S - pfintr */


    	   .text
pferrcode: .long 0
           .globl  pfintr,pferrcode 
pfintr:
	popl pferrcode  	/* pop the pf error code off the stack      */
    pushfl              /* save the eflag register                  */
    cli                 /* disable all interrupts                   */
    pushal              /* save all general purpose registers       */
    call pfint         	/* call high level pf handler code          */
    popal               /* restore all general purpose registers    */
    popfl               /* restore eflag register                   */
	iret                /* return from interrupt handling           */


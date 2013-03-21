/*
void DisableFPUExceptions();
*/

#include <as_reg_compat.h>

	.set push
	.set noreorder
	.globl DisableFPUExceptions
	.ent DisableFPUExceptions
DisableFPUExceptions:
	cfc1	$2, $31
	lui		$8, 0x80
	and		$8, $2, $8	# Mask off all bits except for 23 of FCR
	ctc1	$8, $31
	jr		$31
	nop
	.end DisableFPUExceptions
	.set pop

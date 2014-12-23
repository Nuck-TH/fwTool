

@---------------------------------------------------------------------------------
@ http://problemkaputt.de/gbatek.htm#dsmemorycontrolbios
@---------------------------------------------------------------------------------
@ 05ECh  ldrb r3,[r3,12h]      ;requires incoming r3=src-12h
@ 05EEh  pop  r2,r4,r6,r7,r15  ;requires dummy values & THUMB retadr on stack
@---------------------------------------------------------------------------------
@ after dumping found a better instruction sequence
@---------------------------------------------------------------------------------
@ 0x1664 : ldr r1, [r1] ; str r1, [r0] ; bx lr
@---------------------------------------------------------------------------------
	.arch	armv4t
	.cpu	arm7tdmi


	.text
	.global readBios
@---------------------------------------------------------------------------------
	.arm
	.align	4
	.type	readBios STT_FUNC
@---------------------------------------------------------------------------------
readBios:
@---------------------------------------------------------------------------------
	push	{lr}
	mov	r2, #0
	adr	lr, ret
	mov	r11, r11
readLoop:
	mov	r1, r2
	ldr	r3, =0x1665
	bx	r3
ret:
	add	r0, r0, #4
	add	r2, r2, #4
	cmp	r2, #0x4000
	bne	readLoop

	pop	{lr}
	bx	lr



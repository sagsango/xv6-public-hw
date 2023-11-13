#include "syscall.h"
#include "traps.h"

.global sigret_start
.global sigret_end

sigret_start:
  mov $SYS_sigret, %eax;
  syscall
	ret # This line will never execute, because we will not return form this path see trapasm.S
sigret_end:

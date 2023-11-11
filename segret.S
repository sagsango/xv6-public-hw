#include "syscall.h"
#include "traps.h"

.global sigret_start
.global sigret_end

sigret_start:
  mov $SYS_sigret, %eax;
  syscall
	ret
sigret_end:

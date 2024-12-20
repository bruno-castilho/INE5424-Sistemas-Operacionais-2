// EPOS ARMv8 Time-Stamp Counter Mediator Initialization

#include <machine/machine.h>
#include <machine/timer.h>

__BEGIN_SYS

void TSC::init()
{
    db<Init, TSC>(TRC) << "TSC::init()" << endl;

    if(CPU::id() == CPU::BSP) {
        // Disable counting before programming
        reg(GTCLR) = 0;

        // Set timer to 0
        reg(GTCTRL) = 0;
        reg(GTCTRH) = 0;

        // Re-enable counting
        reg(GTCLR) = 1;
    }
}

__END_SYS

// EPOS IA32 CPU Mediator Initialization

#include <architecture.h>
#include <machine/machine.h>
#include <system.h>

__BEGIN_SYS

void CPU::init()
{
    db<Init, CPU>(TRC) << "CPU::init()" << endl;

    _cpu_clock = System::info()->tm.cpu_clock;
    _cpu_current_clock = System::info()->tm.cpu_clock;
    _bus_clock = System::info()->tm.bus_clock;

    // Initialize the MMU
    if(CPU::id() == CPU::BSP) {
        if(Traits<MMU>::enabled)
            MMU::init();
        else
            db<Init, MMU>(WRN) << "MMU is disabled!" << endl;
    }

    // Initialize the PMU	
    if(Traits<PMU>::enabled){
        PMU::init();
        PMU::config(2, 2);
        PMU::start(2);
        PMU::reset(2);
        PMU::config(3, 15);
        PMU::start(3);
        PMU::reset(3);
        PMU::config(4, 30);
        PMU::start(4);
        PMU::reset(4);
    }
}

void CPU::smp_barrier_init(unsigned int cores) {
    // Core activation in IA32 is handled by the APIC
    _cores = cores;
    if(multicore)
        APIC::remap();
};

__END_SYS

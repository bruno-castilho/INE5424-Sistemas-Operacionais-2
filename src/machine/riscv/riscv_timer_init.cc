// EPOS RISC-V Timer Mediator Initialization

#include <architecture/cpu.h>
#include <machine/timer.h>
#include <machine/ic.h>

__BEGIN_SYS

void Timer::init()
{
    db<Init, Timer>(TRC) << "Timer::init()" << endl;

    assert(CPU::int_disabled());

    if(CPU::id() == CPU::BSP)
        IC::int_vector(IC::INT_SYS_TIMER, int_handler);

    reset();
    IC::enable(IC::INT_SYS_TIMER);
}

__END_SYS

// EPOS PC Timer Mediator Initialization

#include <machine/timer.h>
#include <machine/ic.h>

__BEGIN_SYS

void Timer::init()
{
    db<Init, Timer>(TRC) << "Timer::init()" << endl;

    CPU::int_disable();

    if(CPU::id() == CPU::BSP)
        IC::int_vector(IC::INT_SYS_TIMER, int_handler);

    disable();
    reset();
    enable();

    IC::enable(IC::INT_SYS_TIMER);

    CPU::int_enable();
}

__END_SYS

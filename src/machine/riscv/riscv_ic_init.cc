// EPOS RISC-V Interrupt Controller Initialization

#include <machine/ic.h>
#include <machine/timer.h>

__BEGIN_SYS

void IC::init()
{
    db<Init, IC>(TRC) << "IC::init()" << endl;

    assert(CPU::int_disabled()); // will be reenabled at Thread::init() by Context::load()

    disable(); // will be enabled on demand as handlers are registered

    // Set all exception handlers to exception()
    for(Interrupt_Id i = 0; i < EXCS; i++)
        _int_vector[i] = &exception;

    // Set all interrupt handlers to int_not()
    for(Interrupt_Id i = EXCS; i < INTS; i++)
        _int_vector[i] = &int_not;

    // TODO: map and treat these interrupts (fire non-stop)
    if (Traits<Build>::MODEL == Traits_Tokens::VisionFive2) {
        set_priority(3, 0);
        set_priority(4, 0);
        set_priority(50, 0);
        set_priority(98, 0);
    }

    // Permits all interrupts with non-zero priority
    permit_all_external();
}

__END_SYS

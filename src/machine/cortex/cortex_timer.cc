// EPOS Cortex Timer Mediator Implementation

#include <machine/ic.h>
#include <machine/timer.h>

__BEGIN_SYS

Timer * Timer::_channels[CHANNELS];

#ifdef __raspberry_pi3__
System_Timer_Engine::Count System_Timer_Engine::_count;
bool User_Timer_Engine::_periodic;
User_Timer_Engine::Count User_Timer_Engine::_count;
#endif

void Timer::int_handler(Interrupt_Id i)
{
    if((CPU::id() == CPU::BSP) && _channels[ALARM] && (--_channels[ALARM]->_current[CPU::BSP] <= 0)) {
        _channels[ALARM]->_current[CPU::BSP] = _channels[ALARM]->_initial;
        _channels[ALARM]->_handler(i);
    }

    if(_channels[SCHEDULER] && (--_channels[SCHEDULER]->_current[CPU::id()] <= 0)) {
        _channels[SCHEDULER]->_current[CPU::id()] = _channels[SCHEDULER]->_initial;

        if((Traits<Build>::MODEL != Traits<Build>::Realview_PBX) && (CPU::id() == CPU::BSP))
            for(unsigned int cpu = 1; cpu < CPU::cores(); cpu++)
                IC::ipi(cpu, IC::INT_RESCHEDULER);

        _channels[SCHEDULER]->_handler(i);
    }
}

void Timer::eoi(Interrupt_Id i) {
    Engine::eoi(i);
}

void User_Timer::eoi(Interrupt_Id i) {
    Engine::eoi(i);
}

__END_SYS

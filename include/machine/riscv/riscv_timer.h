// EPOS RISC-V Timer Mediator Declarations

#ifndef __riscv_timer_h
#define __riscv_timer_h

#include <architecture/cpu.h>
#include <machine/ic.h>
#include <machine/timer.h>
#include <system/memory_map.h>
#include <utility/convert.h>

__BEGIN_SYS

// Tick timer used by the system
class Timer: private Timer_Common, private CLINT
{
    friend Machine;
    friend IC;
    friend class Init_System;

protected:
    static const bool multicore = Traits<System>::multicore;
    static const unsigned int CHANNELS = 2;
    static const Hertz FREQUENCY = Traits<Timer>::FREQUENCY;

    typedef IC_Common::Interrupt_Id Interrupt_Id;

public:
    using Timer_Common::Tick;
    using Timer_Common::Handler;

    // Channels
    enum {
        SCHEDULER,
        ALARM
    };

    static const Hertz CLOCK = Traits<Timer>::CLOCK;

protected:
    Timer(unsigned int channel, Hertz frequency, Handler handler, bool retrigger = true)
    : _channel(channel), _initial(FREQUENCY / frequency), _retrigger(retrigger), _handler(handler) {
        db<Timer>(TRC) << "Timer(f=" << frequency << ",h=" << reinterpret_cast<void*>(handler) << ",ch=" << channel << ") => {count=" << _initial << "}" << endl;

        if(_initial && (channel < CHANNELS) && !_channels[channel])
            _channels[channel] = this;
        else
            db<Timer>(WRN) << "Timer not installed!"<< endl;

        for(unsigned int i = 0; i < Traits<Machine>::CPUS; i++)
            _current[i] = _initial;
    }

public:
    ~Timer() {
        db<Timer>(TRC) << "~Timer(f=" << frequency() << ",h=" << reinterpret_cast<void*>(_handler) << ",ch=" << _channel << ") => {count=" << _initial << "}" << endl;

        _channels[_channel] = 0;
    }

    Tick read() { return _current[CPU::id()]; }

    int restart() {
        db<Timer>(TRC) << "Timer::restart() => {f=" << frequency() << ",h=" << reinterpret_cast<void *>(_handler) << ",count=" << _current[CPU::id()] << "}" << endl;

        int percentage = _current[CPU::id()] * 100 / _initial;
        _current[CPU::id()] = _initial;

        return percentage;
    }

    static void reset() { config(FREQUENCY); }
    static void enable() {}
    static void disable() {}

    Hertz frequency() const { return (FREQUENCY / _initial); }
    void frequency(Hertz f) { _initial = FREQUENCY / f; reset(); }

    void handler(Handler handler) { _handler = handler; }

private:
    static void config(Hertz frequency) { mtimecmp(mtime() + (CLOCK / frequency)); }

    static void int_handler(Interrupt_Id i);

    static void init();

protected:
    unsigned int _channel;
    Tick _initial;
    bool _retrigger;
    volatile Tick _current[Traits<Machine>::CPUS];
    Handler _handler;

    static Timer * _channels[CHANNELS];
};

// Timer used by Thread::Scheduler
class Scheduler_Timer: public Timer
{
public:
    Scheduler_Timer(Microsecond quantum, Handler handler): Timer(SCHEDULER, 1000000 / quantum, handler) {}
};

// Timer used by Alarm
class Alarm_Timer: public Timer
{
public:
    Alarm_Timer(Handler handler): Timer(ALARM, FREQUENCY, handler) {}
};

__END_SYS

#endif

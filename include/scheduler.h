// EPOS Scheduler Component Declarations

#ifndef __scheduler_h
#define __scheduler_h

#include <architecture/cpu.h>
#include <architecture/pmu.h>
#include <architecture/tsc.h>
#include <utility/scheduling.h>
#include <utility/math.h>
#include <utility/convert.h>

__BEGIN_SYS

// All scheduling criteria, or disciplines, must define operator int() with
// the semantics of returning the desired order of a given object within the
// scheduling list
class Scheduling_Criterion_Common
{
public:
    // Priorities
    enum : int {
        MAIN   = -1,
        HIGH   = 0,
        NORMAL = (unsigned(1) << (sizeof(int) * 8 - 3)) - 1,
        LOW    = (unsigned(1) << (sizeof(int) * 8 - 2)) - 1,
        IDLE   = (unsigned(1) << (sizeof(int) * 8 - 1)) - 1
    };

    // Constructor helpers
    enum : unsigned int {
        SAME        = 0,
        NOW         = 0,
        UNKNOWN     = 0,
        ANY         = -1U
    };

    // Policy traits
    static const bool timed = false;
    static const bool dynamic = false;
    static const bool preemptive = true;

protected:
    Scheduling_Criterion_Common() {}
};

// Priority (static and dynamic)
class Priority: public Scheduling_Criterion_Common
{
    
public:
    template <typename ... Tn>
    Priority(int p = NORMAL, Tn & ... an): _priority(p) {}

    operator const volatile int() const volatile { return _priority; }

protected:
    volatile int _priority;
};

// Round-Robin
class RR: public Priority
{
public:
    static const bool timed = true;
    static const bool dynamic = false;
    static const bool preemptive = true;

public:
    template <typename ... Tn>
    RR(int p = NORMAL, Tn & ... an): Priority(p) {}
};

// First-Come, First-Served (FIFO)
class FCFS: public Priority
{
public:
    static const bool timed = false;
    static const bool dynamic = false;
    static const bool preemptive = false;

public:
    template <typename ... Tn>
    FCFS(int p = NORMAL, Tn & ... an);
};



__END_SYS

#endif

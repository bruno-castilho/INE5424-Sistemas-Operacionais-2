// EPOS Thread Component Declarations

#ifndef __process_h
#define __process_h

#include <architecture.h>
#include <machine.h>
#include <utility/queue.h>
#include <utility/handler.h>
#include <scheduler.h>

extern "C"
{
    void __exit();
}

__BEGIN_SYS

class Thread
{
    friend class Init_End;            // context->load()
    friend class Init_System;         // for init() on CPU != 0
    friend class Scheduler<Thread>;   // for link()
    friend class Synchronizer_Common; // for lock() and sleep()
    friend class Alarm;               // for lock()
    friend class System;              // for init()

protected:
    static const bool preemptive = Traits<Thread>::Criterion::preemptive;
    static const unsigned int QUANTUM = Traits<Thread>::QUANTUM;
    static const unsigned int STACK_SIZE = Traits<Application>::STACK_SIZE;

    typedef CPU::Log_Addr Log_Addr;
    typedef CPU::Context Context;

public:
    // Thread State
    enum State
    {
        RUNNING,
        READY,
        SUSPENDED,
        WAITING,
        FINISHING
    };

    // Thread Scheduling Criterion
    typedef Traits<Thread>::Criterion Criterion;
    enum
    {
        HIGH = Criterion::HIGH,
        NORMAL = Criterion::NORMAL,
        LOW = Criterion::LOW,
        MAIN = Criterion::MAIN,
        IDLE = Criterion::IDLE
    };

    // Thread Queue
    typedef Ordered_Queue<Thread, Criterion, Scheduler<Thread>::Element> Queue;

    // Thread Configuration
    struct Configuration
    {
        Configuration(State s = READY, Criterion c = NORMAL, unsigned int i = 0, Second d = 0, unsigned int ss = STACK_SIZE)
            : state(s), criterion(c), instructions(i), deadline(d), stack_size(ss) {}

        State state;
        Criterion criterion;
        unsigned int instructions;
        Second deadline;
        unsigned int stack_size;
    };

    static volatile unsigned int _total_instructions;
    static Second _longer_expiration_time;

    int totalSize;
    int totalTime;
    int CPUfreq;
    Thread* leaderHead = nullptr;
    int deadline;
    int instructions;


public:
    template <typename... Tn>
    Thread(int (*entry)(Tn...), Tn... an);
    template <typename... Tn>
    Thread(Configuration conf, int (*entry)(Tn...), Tn... an);
    ~Thread();

    const volatile State &state() const { return _state; }
    Criterion &criterion() { return const_cast<Criterion &>(_link.rank()); }

    const volatile Criterion &priority() const { return _link.rank(); }
    void priority(Criterion p);

    int join();
    void pass();
    void suspend();
    void resume();

    static Thread *volatile self() { return running(); }
    static void yield();
    static void exit(int status = 0);

protected:
    void constructor_prologue(unsigned int stack_size, unsigned int instructions, Second deadline);
    void constructor_epilogue(Log_Addr entry, unsigned int stack_size);
    void increment_thread__count();
    void decrement_thread__count();

    Queue::Element *link() { return &_link; }

    static Thread *volatile running() { return _scheduler.chosen(); }

    static void lock() { CPU::int_disable(); }
    static void unlock() { CPU::int_enable(); }
    static bool locked() { return CPU::int_disabled(); }

    static void sleep(Queue *q);
    static void wakeup(Queue *q);
    static void wakeup_all(Queue *q);

    static void reschedule();
    static void time_slicer(IC::Interrupt_Id interrupt);

    static float calculate_cpu_frequency();
    static void dispatch(Thread *prev, Thread *next, bool charge = true);

    static int idle();

private:
    static void init();

protected:
    char *_stack;
    Context *volatile _context;
    volatile State _state;
    Queue *_waiting;
    Thread *volatile _joining;
    Queue::Element _link;
    Criterion _criterion;
    unsigned int _instructions;
    Second _deadline;
    Second _expiration_time;

    static volatile unsigned int _thread_count;
    static Scheduler_Timer *_timer;
    static Scheduler<Thread> _scheduler;
};

template <typename... Tn>
inline Thread::Thread(int (*entry)(Tn...), Tn... an)
    : _state(READY), _waiting(0), _joining(0), _link(this, NORMAL)
{
    _criterion = NORMAL;

    constructor_prologue(STACK_SIZE, 0, 0);

    _context = CPU::init_stack(0, _stack + STACK_SIZE, &__exit, entry, an...);
    constructor_epilogue(entry, STACK_SIZE);
}

template <typename... Tn>
inline Thread::Thread(Configuration conf, int (*entry)(Tn...), Tn... an)
    : _state(conf.state), _waiting(0), _joining(0), _link(this, conf.criterion)
{
    _criterion = conf.criterion;
    _deadline = conf.deadline;
    _instructions = conf.instructions;
    _expiration_time = conf.deadline;

    constructor_prologue(conf.stack_size, conf.instructions, conf.deadline);
    _context = CPU::init_stack(0, _stack + conf.stack_size, &__exit, entry, an...);
    constructor_epilogue(entry, conf.stack_size);
}

// A Java-like Active Object
class Active : public Thread
{
public:
    Active() : Thread(Configuration(Thread::SUSPENDED), &entry, this) {}
    virtual ~Active() {}

    virtual int run() = 0;

    void start() { resume(); }

private:
    static int entry(Active *runnable) { return runnable->run(); }
};

// An event handler that triggers a thread (see handler.h)
class Thread_Handler : public Handler
{
public:
    Thread_Handler(Thread *h) : _handler(h) {}
    ~Thread_Handler() {}

    void operator()() { _handler->resume(); }

private:
    Thread *_handler;
};

__END_SYS

#endif

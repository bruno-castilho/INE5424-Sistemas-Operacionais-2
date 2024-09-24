// EPOS Thread Implementation

#include <machine.h>
#include <system.h>
#include <time.h>
#include <process.h>

__BEGIN_SYS

extern OStream kout;
extern Clock clock;

volatile unsigned int Thread::_thread_count;
volatile unsigned int Thread::_total_instructions;
Second Thread::_longer_expiration_time;

Scheduler_Timer *Thread::_timer;
Scheduler<Thread> Thread::_scheduler;

float Thread::calculate_cpu_frequency()
{
    float max_frequency = 3.2;
    float max_instructions_per_second = 70;

    Second current_time = clock.now();
    Second available_time = _longer_expiration_time - current_time;
    float instructions_per_second = _total_instructions / available_time;
    float frequency = max_frequency * instructions_per_second / max_instructions_per_second;

    db<Thread>(TRC) << "Thread::calculate_cpu_frequency("
                    << "mfre=" << max_frequency
                    << ",mipm=" << max_instructions_per_second
                    << ",cur=" << current_time
                    << ",ava=" << available_time
                    << ",ipm=" << instructions_per_second
                    << ",fre=" << frequency << ")" << endl;
    return frequency;
}

// Changes CPU frequency used for this thread and neighbours
// void insertion() {

//     // Please don't let it divide by 0 on avaiable time. (to-do)
//     leaderHead = this;

//     Thread* MT = _link._next->_object; // Task whose time will be shrunk

//     if (_link._prev == nullptr) {
//         totalTime = deadline;
//         CPUfreq = totalSize/totalTime;
//         MT = _link._next->object;
//         if (MT != nullptr) {
//             MT->totalTime -= totalTime;
//             MT->CPUfreq = MT->totalSize/MT->totalTime;
//             if (MT != MT->leaderHead) {
//                 MT->leaderHead->totalTime -= totalTime;
//                 MT->leaderHead->CPUfreq = MT->leaderHead->totalSize/MT->leaderHead->totalTime;
//             }
//             MT->dissolve();
//         } else {
//             return;
//         }
//     } else {
//         totalTime = deadline - _link._prev->deadline;
//         CPUfreq = totalSize/totalTime;
//         if (MT != nullptr) {
//             MT->totalTime -= totalTime;
//             MT->CPUfreq = MT->totalSize/MT->totalTime;
//             if (MT != _link._prev->leaderHead and MT != MT->leaderHead) {
//                 MT->leaderHead->totalTime -= totalTime;
//                 MT->leaderHead->CPUfreq = MT->leaderHead->totalSize/MT->leaderHead->totalTime;
//             }
//         }
//         if (MT == nullptr or (_prev != nullptr && _link._prev->leaderHead != MT->leaderHead && _link._prev->leaderHead != MT)) { // Task inserted between two Blocks
//             conquer();
//         } else { // Task inserted between a Block
//             _link._prev->leaderHead->dissolve();
//         }
//     }
// }

// Function that fuses two Blocks, called when LeftBlock (oldLeader)'s frequency is lower than RightBlock (this)'s frequency
// if we have:
// (A, B, C, D, E, F) being tasks
// ABC' being a block of tasks A B C with C' as the leader
// This function does: ABC' DEF' -> ABCDEF'
// Called by newLeader (oldLeader's Right Block)
// void coup(Thread *oldLeader)
// {
//     Thread *head = oldLeader->leaderHead;
//     totalSize += oldLeader->totalSize;
//     totalTime += oldLeader->totalTime;
//     CPUfreq = totalSize / totalTime;
//     oldLeader->totalSize = oldLeader->instructions;
//     if (oldLeader->_link._prev == nullptr)
//     {
//         oldLeader->totaltime = oldLeader->deadline;
//     }
//     else
//     {
//         oldLeader->totaltime = oldLeader->deadline - oldLeader->_prev->_object->deadline;
//     }
//     oldLeader->CPUfreq = oldLeader->totalSize / oldLeader->totalTime;

//     leaderHead = head;

//     while (head != this)
//     {
//         head->leaderHead = this;
//         head = head->_next->_object;
//     }
// }

// called on I'
// will do (ABC' D' EF' GHI') -> (ABC' D' EFGHI') -> (ABC' DEFGHI')
// void conquer()
// {
//     Thread *leftKingdom = leaderHead->_prev->_object; // left block that might be consumed by current block

//     while (leftKingdom != nullptr and leftKingdom->CPUfreq < CPUfreq)
//     {
//         coup(leftKingdom);
//         leftKingdom = leaderHead->_prev;
//     }

//     if (_link._next != nullptr and _link._next->_object->leaderHead->CPUfreq > CPUfreq)
//     {
//         _link._next->_object->leaderHead->coup(this);
//         // newEmperor->leaderHead->_right->_object->conquer(this); Too ineficient for too little gain??
//     }
// }

// ABCHDEFG' -> A' B' C' H' D' E' F' G' -> conquer(G') ^ conquer(F') ^ ...
// Called on Leader G'
// void dissolve()
// {
//     Thread *head = leaderHead;

//     // resseting leader's stats
//     totalSize = instructions;
//     totaltime = deadline - _link._prev->deadline;
//     CPUfreq = totalSize / totalTime;

//     // breaking off each part of Block
//     Thread *temp = this;
//     while (temp != head)
//     {
//         temp->leaderHead = temp;
//         temp = temp->_link._prev->_object;
//     }
//     head->leaderHead = head;

//     // Giving the chance for each new Block to conquer others
//     temp = this;
//     while (temp != nullptr and temp != head)
//     {
//         temp->conquer();
//         temp = temp->leaderHead->_link._prev->_object;
//     }
// }

void Thread::constructor_prologue(unsigned int stack_size, unsigned int instructions, Second deadline)
{
    lock();
    _thread_count++;
    _expiration_time = clock.now() + deadline;
    _total_instructions += instructions;
    _longer_expiration_time = (_longer_expiration_time >= _expiration_time ? _longer_expiration_time : _expiration_time); // Melhorar;

    _scheduler.insert(this);

    _stack = new (SYSTEM) char[stack_size];
}

void Thread::constructor_epilogue(Log_Addr entry, unsigned int stack_size)
{
    db<Thread>(TRC) << "Thread(entry=" << entry
                    << ",state=" << _state
                    << ",priority=" << _link.rank()
                    << ",stack={b=" << reinterpret_cast<void *>(_stack)
                    << ",s=" << stack_size
                    << "},context={b=" << _context
                    << "," << *_context << "}) => " << this << endl;

    assert((_state != WAITING) && (_state != FINISHING)); // invalid states

    if ((_state != READY) && (_state != RUNNING))
        _scheduler.suspend(this);

    if (preemptive && (_state == READY) && (_link.rank() != IDLE))
        reschedule();

    // insertion();

    unlock();
}

Thread::~Thread()
{
    lock();

    db<Thread>(TRC) << "~Thread(this=" << this
                    << ",state=" << _state
                    << ",priority=" << _link.rank()
                    << ",stack={b=" << reinterpret_cast<void *>(_stack)
                    << ",context={b=" << _context
                    << "," << *_context << "})" << endl;

    // The running thread cannot delete itself!
    assert(_state != RUNNING);

    switch (_state)
    {
    case RUNNING: // For switch completion only: the running thread would have deleted itself! Stack wouldn't have been released!
        exit(-1);
        break;
    case READY:
        _scheduler.remove(this);
        _thread_count--;
        break;
    case SUSPENDED:
        _scheduler.resume(this);
        _scheduler.remove(this);
        _thread_count--;
        break;
    case WAITING:
        _waiting->remove(this);
        _scheduler.resume(this);
        _scheduler.remove(this);
        _thread_count--;
        break;
    case FINISHING: // Already called exit()
        break;
    }

    if (_joining)
        _joining->resume();

    unlock();

    delete _stack;
}

void Thread::priority(Criterion c)
{
    lock();

    db<Thread>(TRC) << "Thread::priority(this=" << this << ",prio=" << c << ")" << endl;

    if (_state != RUNNING)
    { // reorder the scheduling queue
        _scheduler.suspend(this);
        _link.rank(c);
        _scheduler.resume(this);
    }
    else
        _link.rank(c);

    if (preemptive)
        reschedule();

    unlock();
}

int Thread::join()
{
    lock();

    db<Thread>(TRC) << "Thread::join(this=" << this << ",state=" << _state << ")" << endl;

    // Precondition: no Thread::self()->join()
    assert(running() != this);

    // Precondition: a single joiner
    assert(!_joining);

    if (_state != FINISHING)
    {
        Thread *prev = running();

        _joining = prev;
        prev->_state = SUSPENDED;
        _scheduler.suspend(prev); // implicitly choose() if suspending chosen()

        Thread *next = _scheduler.chosen();

        dispatch(prev, next);
    }

    unlock();

    return *reinterpret_cast<int *>(_stack);
}

void Thread::pass()
{
    lock();

    db<Thread>(TRC) << "Thread::pass(this=" << this << ")" << endl;

    Thread *prev = running();
    Thread *next = _scheduler.choose(this);

    if (next)
        dispatch(prev, next, false);
    else
        db<Thread>(WRN) << "Thread::pass => thread (" << this << ") not ready!" << endl;

    unlock();
}

void Thread::suspend()
{
    lock();

    db<Thread>(TRC) << "Thread::suspend(this=" << this << ")" << endl;

    Thread *prev = running();

    _state = SUSPENDED;
    _scheduler.suspend(this);

    Thread *next = _scheduler.chosen();

    dispatch(prev, next);

    unlock();
}

void Thread::resume()
{
    lock();

    db<Thread>(TRC) << "Thread::resume(this=" << this << ")" << endl;

    if (_state == SUSPENDED)
    {
        _state = READY;
        _scheduler.resume(this);

        if (preemptive)
            reschedule();
    }
    else
        db<Thread>(WRN) << "Resume called for unsuspended object!" << endl;

    unlock();
}

void Thread::yield()
{
    lock();

    // db<Thread>(TRC) << "Thread::yield(running=" << running() << ")" << endl;

    Thread *prev = running();
    Thread *next = _scheduler.choose_another();

    dispatch(prev, next);

    unlock();
}

void Thread::exit(int status)
{
    lock();

    db<Thread>(TRC) << "Thread::exit(status=" << status << ") [running=" << running() << "]" << endl;

    Thread *prev = running();
    _scheduler.remove(prev);
    prev->_state = FINISHING;
    *reinterpret_cast<int *>(prev->_stack) = status;

    _thread_count--;
    _total_instructions -= prev->_instructions;

    if (prev->_joining)
    {
        prev->_joining->_state = READY;
        _scheduler.resume(prev->_joining);
        prev->_joining = 0;
    }

    Thread *next = _scheduler.choose(); // at least idle will always be there

    dispatch(prev, next);

    unlock();
}

void Thread::sleep(Queue *q)
{
    db<Thread>(TRC) << "Thread::sleep(running=" << running() << ",q=" << q << ")" << endl;

    assert(locked()); // locking handled by caller

    Thread *prev = running();
    _scheduler.suspend(prev);
    prev->_state = WAITING;
    prev->_waiting = q;
    _total_instructions -= prev->_instructions;

    q->insert(&prev->_link);

    Thread *next = _scheduler.chosen();

    dispatch(prev, next);
}

void Thread::wakeup(Queue *q)
{
    db<Thread>(TRC) << "Thread::wakeup(running=" << running() << ",q=" << q << ")" << endl;

    assert(locked()); // locking handled by caller

    if (!q->empty())
    {
        Thread *t = q->remove()->object();
        t->_state = READY;
        t->_waiting = 0;
        _total_instructions += t->_instructions;
        t->_expiration_time = clock.now() + t->_deadline;
        _longer_expiration_time = (_longer_expiration_time >= t->_expiration_time ? _longer_expiration_time : t->_expiration_time); // Melhorar;

        _scheduler.resume(t);

        if (preemptive)
            reschedule();
    }
}

void Thread::wakeup_all(Queue *q)
{
    db<Thread>(TRC) << "Thread::wakeup_all(running=" << running() << ",q=" << q << ")" << endl;

    assert(locked()); // locking handled by caller

    if (!q->empty())
    {
        while (!q->empty())
        {
            Thread *t = q->remove()->object();
            t->_state = READY;
            t->_waiting = 0;
            _scheduler.resume(t);
        }

        if (preemptive)
            reschedule();
    }
}

void Thread::reschedule()
{
    if (!Criterion::timed || Traits<Thread>::hysterically_debugged)
        db<Thread>(TRC) << "Thread::reschedule()" << endl;

    assert(locked()); // locking handled by caller

    Thread *prev = running();
    Thread *next = _scheduler.choose();

    dispatch(prev, next);
}

void Thread::time_slicer(IC::Interrupt_Id i)
{
    lock();
    reschedule();
    unlock();
}

void Thread::dispatch(Thread *prev, Thread *next, bool charge)
{
    // "next" is not in the scheduler's queue anymore. It's already "chosen"

    if (charge && Criterion::timed)
        _timer->restart();

    if (prev != next)
    {
        if (prev->_state == RUNNING)
            prev->_state = READY;
        next->_state = RUNNING;

        calculate_cpu_frequency();

        db<Thread>(TRC) << "Thread::dispatch(prev=" << prev << ",next=" << next << ")" << endl;
        if (Traits<Thread>::debugged && Traits<Debug>::info)
        {
            CPU::Context tmp;
            tmp.save();
            db<Thread>(INF) << "Thread::dispatch:prev={" << prev << ",ctx=" << tmp << "}" << endl;
        }
        db<Thread>(INF) << "Thread::dispatch:next={" << next << ",ctx=" << *next->_context << "}" << endl;

        // The non-volatile pointer to volatile pointer to a non-volatile context is correct
        // and necessary because of context switches, but here, we are locked() and
        // passing the volatile to switch_constext forces it to push prev onto the stack,
        // disrupting the context (it doesn't make a difference for Intel, which already saves
        // parameters on the stack anyway).
        CPU::switch_context(const_cast<Context **>(&prev->_context), next->_context);
    }
}

int Thread::idle()
{
    db<Thread>(TRC) << "Thread::idle(this=" << running() << ")" << endl;

    while (_thread_count > 1)
    { // someone else besides idle
        if (Traits<Thread>::trace_idle)
            db<Thread>(TRC) << "Thread::idle(this=" << running() << ")" << endl;

        CPU::int_enable();
        CPU::halt();

        if (!preemptive)
            yield();
    }

    kout << "\n\n*** The last thread under control of EPOS has finished." << endl;
    kout << "*** EPOS is shutting down!" << endl;
    Machine::reboot();

    return 0;
}

__END_SYS

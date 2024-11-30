// EPOS Thread Implementation
#include <machine.h>
#include <system.h>
#include <time.h>
#include <process.h>
#include <synchronizer.h>

__BEGIN_SYS

extern OStream kout;

bool Thread::_not_booting;
volatile unsigned int Thread::_thread_count;
volatile unsigned int Thread::_changes_cout;
volatile unsigned int Thread::_cpu_thread_count[Traits<Machine>::CPUS];
volatile Thread* Thread::_cpu_threads[Traits<Machine>::CPUS][Traits<Machine>::MAX_THREADS];
volatile unsigned long long Thread::_cpu_last_dispatch[Traits<Machine>::CPUS];
volatile unsigned long long  Thread::_cpu_instructions_per_second[Traits<Machine>::CPUS];

volatile unsigned long long  Thread::_cpu_instructions_per_second_required[Traits<Machine>::CPUS];
volatile unsigned long long  Thread::_cpu_branch_missprediction[Traits<Machine>::CPUS];

Mutex Thread::_cpu_instructions_per_second_required_mutex[Traits<Machine>::CPUS];
Mutex Thread::_cpu_branch_missprediction_mutex[Traits<Machine>::CPUS];


Scheduler_Timer *Thread::_timer;
Scheduler<Thread> Thread::_scheduler;
Spin Thread::_lock;



void Thread::change_thread_queue_if_necessary(){
    db<Thread>(TRC) << "Thread::change_thread_queue_if_necessary(cpu=" << CPU::id() << ",this=" << running() << ")" << endl;
    unsigned int cpu_selected = select_cpu_by_branch_missprediction();
    unsigned long long cpu_selected_branch_missprediction = _cpu_branch_missprediction[cpu_selected];

    unsigned int current_cpu = CPU::id();

    if(cpu_selected == current_cpu) return;

    for(unsigned int i = 0; i < _cpu_thread_count[current_cpu]; i++){
        Thread* t = const_cast<Thread* volatile>(_cpu_threads[current_cpu][i]);
        unsigned long long cpu_selected_branch_missprediction_predict = cpu_selected_branch_missprediction + t->statistics().branch_misprediction;
        if(cpu_selected_branch_missprediction_predict < _cpu_branch_missprediction[current_cpu]){
            t->decrease_cost();
            t->criterion().queue(cpu_selected);
            t->increase_cost();
            _changes_cout++;
            return;
        }
    }
}

Hertz Thread::calculate_frequency(){
    Hertz current_frequency = CPU::clock();
    unsigned long long instructions_per_second = _cpu_instructions_per_second[CPU::id()];
    unsigned long long instructions_per_second_required = _cpu_instructions_per_second_required[CPU::id()];

    return (instructions_per_second ==  0 ? current_frequency : current_frequency * instructions_per_second_required / instructions_per_second);
}

unsigned int Thread::get_changes_cout(){
    return _changes_cout;
}

unsigned long long Thread::get_instructions_per_second(unsigned int cpu){
    return _cpu_instructions_per_second[cpu];
}

unsigned long long Thread::get_instructions_per_second_required(unsigned int cpu){
    return _cpu_instructions_per_second_required[cpu];
}

unsigned long long Thread::get_branch_misprediction(unsigned int cpu){
    return _cpu_branch_missprediction[cpu];
}

unsigned int Thread::get_thread_count(unsigned int cpu){
    return _cpu_thread_count[cpu];
}

bool  Thread::check_threads(unsigned int cpu){
    bool its_clean = true;
    for(unsigned int j = 0; j < Traits<Machine>::MAX_THREADS; j++){
        if(_cpu_threads[cpu][j] != nullptr) its_clean = false;
    }
    return its_clean;
}

unsigned int Thread::select_cpu_by_instructions_per_second(){
    unsigned int cpu_selected = 0;
    unsigned long long current_is = _cpu_instructions_per_second_required[cpu_selected];

    for(unsigned int cpu = 1; cpu < Traits<Machine>::CPUS; cpu++){
        unsigned long long is = _cpu_instructions_per_second_required[cpu];
        if(is < current_is){
            cpu_selected = cpu;
            current_is = is;
        }
    }

    return cpu_selected;
}

unsigned int Thread::select_cpu_by_branch_missprediction(){
    unsigned int cpu_selected = 0;
    unsigned long long current_is = _cpu_branch_missprediction[cpu_selected];

    for(unsigned int cpu = 1; cpu < Traits<Machine>::CPUS; cpu++){
        unsigned long long is = _cpu_branch_missprediction[cpu];
        if(is < current_is){
            cpu_selected = cpu;
            current_is = is;
        }
    }

    return cpu_selected;
}

void Thread::increase_cost(){
    instructions_per_second = statistics().instructions_retired * 1000000ULL / criterion().period();
    branch_misprediction = statistics().branch_misprediction;

    _cpu_instructions_per_second_required[criterion().queue()] += instructions_per_second;
    _cpu_branch_missprediction[criterion().queue()] += branch_misprediction;

    _cpu_threads[this->criterion().queue()][_cpu_thread_count[this->criterion().queue()]] = this;
    _cpu_thread_count[this->criterion().queue()] += 1;

}

void Thread::decrease_cost(){
    _cpu_instructions_per_second_required[criterion().queue()] -= instructions_per_second;
    _cpu_branch_missprediction[criterion().queue()] -= branch_misprediction;

    for(unsigned int i = 0; i < _cpu_thread_count[criterion().queue()]; i++){
        if(_cpu_threads[criterion().queue()][i] == this){
            _cpu_threads[criterion().queue()][i] = nullptr;
            for(unsigned int j = i + 1; j < _cpu_thread_count[criterion().queue()]; j++){
                _cpu_threads[criterion().queue()][j-1] = _cpu_threads[criterion().queue()][j];
                _cpu_threads[criterion().queue()][j] = nullptr;
            }
            _cpu_thread_count[criterion().queue()] -= 1;
            break;
        }
    }
}

void Thread::update_cost(){
    _cpu_instructions_per_second_required[criterion().queue()] -= instructions_per_second;
     _cpu_branch_missprediction[criterion().queue()] -= branch_misprediction;

    instructions_per_second = statistics().instructions_retired * 1000000ULL / criterion().period();
    branch_misprediction = statistics().branch_misprediction;

    _cpu_instructions_per_second_required[criterion().queue()] += instructions_per_second;
    _cpu_branch_missprediction[criterion().queue()] += branch_misprediction;
}

void Thread::constructor_prologue(unsigned int stack_size)
{
    lock();
    _thread_count++;
    _scheduler.insert(this);
    _stack = new (SYSTEM) char[stack_size];
}

void Thread::constructor_epilogue(Log_Addr entry, unsigned int stack_size) {
    db<Thread>(TRC) << "Thread(entry=" << entry
                    << ",state=" << _state
                    << ",priority=" << _link.rank()
                    << ",stack={b=" << reinterpret_cast<void *>(_stack)
                    << ",s=" << stack_size
                    << "},context={b=" << _context
                    << "," << *_context << "}) => " << this << "@" << _link.rank().queue() << endl;

    assert((_state != WAITING) && (_state != FINISHING)); // invalid states

    if (_link.rank() != IDLE)
        _task->enroll(this);

    if ((_state != READY) && (_state != RUNNING))
        _scheduler.suspend(this);

    criterion().handle(Criterion::CREATE);

    if(preemptive && (_state == READY) && (_link.rank() != IDLE))
        reschedule(_link.rank().queue());

    if(_link.rank() != IDLE && _link.rank() != MAIN)
        increase_cost();
    

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

    _task->dismiss(this);

    if (_joining)
        _joining->resume();

    unlock();

    delete _stack;
}

void Thread::priority(Criterion c)
{
    lock();

    db<Thread>(TRC) << "Thread::priority(this=" << this << ",prio=" << c << ")" << endl;

    unsigned long old_cpu = _link.rank().queue();
    unsigned long new_cpu = c.queue();

    if(_state != RUNNING) { // reorder the scheduling queue

        _scheduler.suspend(this);
        _link.rank(c);
        _scheduler.resume(this);
    }
    else
        _link.rank(c);

    if(preemptive) {
    	if(smp) {
    	    if(old_cpu != CPU::id())
    	        reschedule(old_cpu);
    	    if(new_cpu != CPU::id())
    	        reschedule(new_cpu);
    	} else
    	    reschedule();
    }

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

    if (_state == SUSPENDED) {
        _state = READY;
        _scheduler.resume(this);
        if(preemptive)
            reschedule(_link.rank().queue());
    } else

        db<Thread>(WRN) << "Resume called for unsuspended object!" << endl;


    unlock();
}

void Thread::yield()
{
    lock();

    db<Thread>(TRC) << "Thread::yield(running=" << running() << ")" << endl;

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
    prev->criterion().handle(Criterion::FINISH);
    prev->decrease_cost();

    _thread_count--;

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

        _scheduler.resume(t);

        if(preemptive)
            reschedule(t->_link.rank().queue());

    }
}

void Thread::wakeup_all(Queue *q)
{
    db<Thread>(TRC) << "Thread::wakeup_all(running=" << running() << ",q=" << q << ")" << endl;

    assert(locked()); // locking handled by caller

    if(!q->empty()) {
        assert(Criterion::QUEUES <= sizeof(unsigned long) * 8);
        unsigned long cpus = 0;
        while(!q->empty()) {
            Thread * t = q->remove()->object();

            t->_state = READY;
            t->_waiting = 0;
            _scheduler.resume(t);
            cpus |= 1 << t->_link.rank().queue();
        }
        if(preemptive) {
            for(unsigned long i = 0; i < Criterion::QUEUES; i++)
                if(cpus & (1 << i))
                    reschedule(i);
        }
    }
}

void Thread::reschedule()

{
    if(!Criterion::timed || Traits<Thread>::hysterically_debugged)
        db<Thread>(TRC) << "Thread::reschedule()" << endl;

    assert(locked()); // locking handled by caller
    Thread * prev = running();
    Thread * next = _scheduler.choose();

    dispatch(prev, next);
}

void Thread::reschedule(unsigned int cpu)
{
    assert(locked()); // locking handled by caller

    if(!smp || (cpu == CPU::id()))
        reschedule();
    else {
        db<Thread>(TRC) << "Thread::reschedule(cpu=" << cpu << ")" << endl;
        IC::ipi(cpu, IC::INT_RESCHEDULER);
    }
}

void Thread::rescheduler(IC::Interrupt_Id i)
{
    lock();
    reschedule();
    unlock();
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

        PMU::stop(2);
        PMU::stop(3);
        PMU::stop(4);
        
        unsigned long long current_time = Alarm::elapsed();
        unsigned long long instructions = PMU::read(2);

        if(_cpu_last_dispatch[CPU::id()] != 0){
            unsigned long long time_between_dispatch = current_time - _cpu_last_dispatch[CPU::id()];
            _cpu_instructions_per_second[CPU::id()] = time_between_dispatch != 0 ? instructions * (1000000ULL / time_between_dispatch) : instructions * 1000000ULL;
        }
        _cpu_last_dispatch[CPU::id()] = current_time;
        
        if (Criterion::dynamic)
        {
            prev->criterion().handle(Criterion::CHARGE | Criterion::LEAVE);
            for_all_threads(Criterion::UPDATE);
            next->criterion().handle(Criterion::AWARD | Criterion::ENTER);
        }

        if (prev->_state == RUNNING)
            prev->_state = READY;


        next->_state = RUNNING;

        db<Thread>(TRC) << "Thread::dispatch(prev=" << prev << ",next=" << next << ")" << endl;
        if (Traits<Thread>::debugged && Traits<Debug>::info)
        {
            CPU::Context tmp;
            tmp.save();
            db<Thread>(INF) << "Thread::dispatch:prev={" << prev << ",ctx=" << tmp << "}" << endl;
        }
        db<Thread>(INF) << "Thread::dispatch:next={" << next << ",ctx=" << *next->_context << "}" << endl;
        if(smp)
            _lock.release();

        

        CPU::clock(Thread::calculate_frequency());
        
        PMU::reset(2);
        PMU::reset(3);
        PMU::reset(4);
        PMU::start(2);
        PMU::start(3);
        PMU::start(4);

        // The non-volatile pointer to volatile pointer to a non-volatile context is correct
        // and necessary because of context switches, but here, we are locked() and
        // passing the volatile to switch_constext forces it to push prev onto the stack,
        // disrupting the context (it doesn't make a difference for Intel, which already saves
        // parameters on the stack anyway).
        CPU::switch_context(const_cast<Context **>(&prev->_context), next->_context);

        if(smp)
            _lock.acquire();
    }
}

int Thread::idle()
{
    db<Thread>(TRC) << "Thread::idle(cpu=" << CPU::id() << ",this=" << running() << ")" << endl;
    change_thread_queue_if_necessary();
    while(_thread_count > CPU::cores()) { // someone else besides idles
        if(Traits<Thread>::trace_idle)
            db<Thread>(TRC) << "Thread::idle(cpu=" << CPU::id() << ",this=" << running() << ")" << endl;


        CPU::int_enable();
        CPU::halt();

        if(_scheduler.schedulables() > 0) // a thread might have been woken up by another CPU
            yield();
    }

    if(CPU::id() == CPU::BSP) {
        kout << "\n\n*** The last thread under control of EPOS has finished." << endl;
        kout << "*** EPOS is shutting down!" << endl;
    }

    CPU::smp_barrier();
    Machine::reboot();

    return 0;
}

__END_SYS

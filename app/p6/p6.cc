// EPOS Periodic Thread Component Test Program

#include <time.h>
#include <real-time.h>
#include <synchronizer.h>
#include <utility/geometry.h>

using namespace EPOS;

const unsigned int iterations = 100;
const Milisecond period_a = 100;
const Milisecond period_b = 80;
const Milisecond period_c = 60;
const Milisecond period_d = 150;
const Milisecond period_e = 180;
const Milisecond period_f = 200;
const Milisecond wcet_a = 50;
const Milisecond wcet_b = 20;
const Milisecond wcet_c = 10;
const Milisecond wcet_d = 20;
const Milisecond wcet_e = 10;
const Milisecond wcet_f = 50;

int func_a();
int func_b();
int func_c();
int func_d();
int func_e();
int func_f();

OStream cout;
Chronometer chrono;

Semaphore write(1);

Periodic_Thread * thread_a;
Periodic_Thread * thread_b;
Periodic_Thread * thread_c;
Periodic_Thread * thread_d;
Periodic_Thread * thread_e;
Periodic_Thread * thread_f;
 
Point<long, 2> p, p1(2131231, 123123), p2(2, 13123), p3(12312, 123123);

unsigned long base_loop_count;

void callibrate()
{
    chrono.start();
    Microsecond end = chrono.read() + Microsecond(1000000UL);

    base_loop_count = 0;

    while(chrono.read() < end) {
        p = p + Point<long, 2>::trilaterate(p1, 123123, p2, 123123, p3, 123123);
        base_loop_count++;
    }

    chrono.stop();

    base_loop_count /= 1000;
}

inline void exec(char c, Milisecond time = 0)
{
    Milisecond elapsed = chrono.read() / 1000;

    write.p();
    cout << "\n" << elapsed << " " << c << "-1" << endl
         <<  " A={i=" << thread_a->priority() << ",d=" << thread_a->criterion().deadline() / Alarm::frequency() << ",i="  << thread_a->statistics().instructions_retired << ",m="  << thread_a->statistics().branch_misprediction << "}" << endl
         <<  " B={i=" << thread_b->priority() << ",d=" << thread_b->criterion().deadline() / Alarm::frequency() << ",i="  << thread_b->statistics().instructions_retired << ",m="  << thread_b->statistics().branch_misprediction << "}" << endl
         <<  " C={i=" << thread_c->priority() << ",d=" << thread_c->criterion().deadline() / Alarm::frequency() << ",i="  << thread_c->statistics().instructions_retired << ",m="  << thread_c->statistics().branch_misprediction << "}" << endl
         <<  " D={i=" << thread_d->priority() << ",d=" << thread_d->criterion().deadline() / Alarm::frequency() << ",i="  << thread_d->statistics().instructions_retired << ",m="  << thread_d->statistics().branch_misprediction << "}" << endl
         <<  " E={i=" << thread_e->priority() << ",d=" << thread_e->criterion().deadline() / Alarm::frequency() << ",i="  << thread_e->statistics().instructions_retired << ",m="  << thread_e->statistics().branch_misprediction << "}" << endl
         <<  " F={i=" << thread_f->priority() << ",d=" << thread_f->criterion().deadline() / Alarm::frequency() << ",i="  << thread_f->statistics().instructions_retired << ",m="  << thread_f->statistics().branch_misprediction << "}" << endl;

    cout <<  "\nCPU={id="<<  CPU::id()
         << ",curf=" <<  CPU::clock() 
         << ",minf=" <<  CPU::min_clock() 
         << ",maxf=" << CPU::max_clock() 
         << "}" << endl;


    cout << "_cpu_instructions_per_second" << endl;
    for(unsigned int cpu = 0; cpu < Traits<Machine>::CPUS; cpu++){
        cout << "{" << cpu << ": " << Periodic_Thread::get_instructions_per_second(cpu) << "}" << endl;
    }

    cout << "_cpu_instructions_per_second_required" << endl;
    for(unsigned int cpu = 0; cpu < Traits<Machine>::CPUS; cpu++){
        cout << "{" << cpu << ": " << Periodic_Thread::get_instructions_per_second_required(cpu) << "}" << endl;
    }

    cout << "_cpu_branch_misprediction" << endl;
    for(unsigned int cpu = 0; cpu < Traits<Machine>::CPUS; cpu++){
        cout << "{" << cpu << ": " << Periodic_Thread::get_branch_misprediction(cpu) << "}" << endl;
    }

    write.v();

    for(unsigned long i = 0; i < time; i++)
        for(unsigned long j = 0; j < base_loop_count; j++) {
            p = p + Point<long, 2>::trilaterate(p1, 123123, p2, 123123, p3, 123123);
    }

    write.p();
    elapsed = chrono.read() / 1000;
    cout << "\n" << elapsed << " " << c << "-2" << endl
         <<  " A={i=" << thread_a->priority() << ",d=" << thread_a->criterion().deadline() / Alarm::frequency() << ",i="  << thread_a->statistics().instructions_retired << ",m="  << thread_a->statistics().branch_misprediction << "}" << endl
         <<  " B={i=" << thread_b->priority() << ",d=" << thread_b->criterion().deadline() / Alarm::frequency() << ",i="  << thread_b->statistics().instructions_retired << ",m="  << thread_b->statistics().branch_misprediction << "}" << endl
         <<  " C={i=" << thread_c->priority() << ",d=" << thread_c->criterion().deadline() / Alarm::frequency() << ",i="  << thread_c->statistics().instructions_retired << ",m="  << thread_c->statistics().branch_misprediction << "}" << endl
         <<  " D={i=" << thread_d->priority() << ",d=" << thread_d->criterion().deadline() / Alarm::frequency() << ",i="  << thread_d->statistics().instructions_retired << ",m="  << thread_d->statistics().branch_misprediction << "}" << endl
         <<  " E={i=" << thread_e->priority() << ",d=" << thread_e->criterion().deadline() / Alarm::frequency() << ",i="  << thread_e->statistics().instructions_retired << ",m="  << thread_e->statistics().branch_misprediction << "}" << endl
         <<  " F={i=" << thread_f->priority() << ",d=" << thread_f->criterion().deadline() / Alarm::frequency() << ",i="  << thread_f->statistics().instructions_retired << ",m="  << thread_f->statistics().branch_misprediction << "}" << endl;
    
    write.v();
}


int main()
{
    cout << "Periodic Thread Component Test" << endl;

    cout << "\nThis test consists in creating three periodic threads as follows:" << endl;
    cout << "- Every " << period_a << "ms, thread A executes \"a\" for " << wcet_a << "ms;" << endl;
    cout << "- Every " << period_b << "ms, thread B executes \"b\" for " << wcet_b << "ms;" << endl;
    cout << "- Every " << period_c << "ms, thread C executes \"c\" for " << wcet_c << "ms;" << endl;

    cout << "\nCallibrating the duration of the base execution loop: ";
    callibrate();
    cout << base_loop_count << " iterations per ms!" << endl;


    cout << "\nThreads will now be created and I'll wait for them to finish..." << endl;


    // p,d,c,act,t
    thread_a = new Periodic_Thread(RTConf(period_a * 1000, period_a * 1000, wcet_a * 1000, 0, iterations), &func_a);
    thread_b = new Periodic_Thread(RTConf(period_b * 1000, period_b * 1000, wcet_b * 1000, 0, iterations), &func_b);
    thread_c = new Periodic_Thread(RTConf(period_c * 1000, period_c * 1000, wcet_c * 1000, 0, iterations), &func_c);
    thread_d = new Periodic_Thread(RTConf(period_d * 1000, period_d * 1000, wcet_d * 1000, 0, iterations), &func_d);
    thread_e = new Periodic_Thread(RTConf(period_e * 1000, period_e * 1000, wcet_e * 1000, 0, iterations), &func_e);
    thread_f = new Periodic_Thread(RTConf(period_f * 1000, period_f * 1000, wcet_f * 1000, 0, iterations), &func_f);


    exec('M');

    chrono.reset();
    chrono.start();

    int status_a = thread_a->join();
    int status_b = thread_b->join();
    int status_c = thread_c->join();
    int status_d = thread_d->join();
    int status_e = thread_e->join();
    int status_f = thread_f->join();

    chrono.stop();

    exec('M');

    for(unsigned int i = 0; i < Traits<Machine>::CPUS; i++){
        cout << "cpu-" << i << " thread_count: " << Periodic_Thread::get_thread_count(i) << endl;
    }

    for(unsigned int i = 0; i < Traits<Machine>::CPUS; i++){

        cout << "cpu-" << i << (Periodic_Thread::check_threads(i) ? " threads its clean" : " It's not clean") << endl;
    }


    cout << "\n... done!" << endl;
    cout << "\n\nThread A exited with status \"" << char(status_a)
         << "\", thread B exited with status \"" << char(status_b)
         << "\" and thread C exited with status \"" << char(status_c) 
         << "\" and thread D exited with status \"" << char(status_d) 
         << "\" and thread E exited with status \"" << char(status_e) 
         << "\" and thread F exited with status \"" << char(status_f) 
         << "." << endl;

    cout << "\nThe estimated time to run the test was "
         << Math::max(period_a, period_b, period_c) * iterations
         << " ms. The measured time was " << chrono.read() / 1000 <<" ms!" << endl;

    cout << Periodic_Thread::get_changes_cout() << " threads were replaced." << endl;
         
    cout << "I'm also done, bye!" << endl;

    return 0;
}

int func_a()
{
    exec('A');

    do {
        exec('a', wcet_a);

    } while (Periodic_Thread::wait_next());
    exec('A');

    return 'A';
}

int func_b()
{
    exec('B');

    do {
        exec('b', wcet_b);
    } while (Periodic_Thread::wait_next());

    exec('B');

    return 'B';
}

int func_c()
{
    exec('C');

    do {
        exec('c', wcet_c);
    } while (Periodic_Thread::wait_next());

    exec('C');

    return 'C';
}

int func_d()
{
    exec('D');

    do {
        exec('d', wcet_d);
    } while (Periodic_Thread::wait_next());

    exec('D');

    return 'D';
}

int func_e()
{
    exec('E');

    do {
        exec('e', wcet_e);
    } while (Periodic_Thread::wait_next());

    exec('E');

    return 'E';
}

int func_f()
{
    exec('F');

    do {
        exec('f', wcet_f);
    } while (Periodic_Thread::wait_next());

    exec('F');

    return 'F';
}
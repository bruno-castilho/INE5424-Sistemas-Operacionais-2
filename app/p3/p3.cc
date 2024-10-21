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
const Milisecond wcet_a = 50;
const Milisecond wcet_b = 20;
const Milisecond wcet_c = 10;

int func_a();
int func_b();
int func_c();

OStream cout;
Chronometer chrono;

Semaphore write(1);

Periodic_Thread * thread_a;
Periodic_Thread * thread_b;
Periodic_Thread * thread_c;
 
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
    cout <<  "\nCPU={id="<<  CPU::id() << "}"
        << endl;


    cout <<  "\nFREQUENCY={maxf="<<  CPU::clock() 
         << ",minf=" <<  CPU::min_clock() 
         << ",curf=" << CPU::max_clock() 
         << "}"
         << endl;
    

    cout << "\n" << elapsed << " " << c << "-1"
         << " [A={i=" << thread_a->priority() << ",d=" << thread_a->criterion().deadline() / Alarm::frequency() << ",a=" << thread_a->leaderHead->avaliable_time << ",b=" << thread_a->leaderHead->block_size << ",f="  << thread_a->leaderHead->frequency << "}"
         <<  " B={i=" << thread_b->priority() << ",d=" << thread_b->criterion().deadline() / Alarm::frequency() << ",a=" << thread_b->leaderHead->avaliable_time << ",b=" << thread_b->leaderHead->block_size << ",f="  << thread_b->leaderHead->frequency << "}"
         <<  " C={i=" << thread_c->priority() << ",d=" << thread_c->criterion().deadline() / Alarm::frequency() << ",a=" << thread_c->leaderHead->avaliable_time << ",b=" << thread_c->leaderHead->block_size << ",f="  << thread_c->leaderHead->frequency << "}]"
        << endl;

    write.v();

    for(unsigned long i = 0; i < time; i++)
        for(unsigned long j = 0; j < base_loop_count; j++) {
            p = p + Point<long, 2>::trilaterate(p1, 123123, p2, 123123, p3, 123123);
    }

    elapsed = chrono.read() / 1000;

    write.p();
    cout << "\n" << elapsed << " " << c << "-2"
         << " [A={i=" << thread_a->priority() << ",d=" << thread_a->criterion().deadline() / Alarm::frequency() << ",a=" << thread_a->leaderHead->avaliable_time << ",b=" << thread_a->leaderHead->block_size << ",f="  << thread_a->leaderHead->frequency << "}"
         <<  " B={i=" << thread_b->priority() << ",d=" << thread_b->criterion().deadline() / Alarm::frequency() << ",a=" << thread_b->leaderHead->avaliable_time << ",b=" << thread_b->leaderHead->block_size << ",f="  << thread_b->leaderHead->frequency << "}"
         <<  " C={i=" << thread_c->priority() << ",d=" << thread_c->criterion().deadline() / Alarm::frequency() << ",a=" << thread_c->leaderHead->avaliable_time << ",b=" << thread_c->leaderHead->block_size << ",f="  << thread_c->leaderHead->frequency << "}]"
         << endl;
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

    exec('M');

    chrono.reset();
    chrono.start();

    int status_a = thread_a->join();
    int status_b = thread_b->join();
    int status_c = thread_c->join();

    chrono.stop();

    exec('M');

    cout << "\n... done!" << endl;
    cout << "\n\nThread A exited with status \"" << char(status_a)
         << "\", thread B exited with status \"" << char(status_b)
         << "\" and thread C exited with status \"" << char(status_c) << "." << endl;

    cout << "\nThe estimated time to run the test was "
         << Math::max(period_a, period_b, period_c) * iterations
         << " ms. The measured time was " << chrono.read() / 1000 <<" ms!" << endl;

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
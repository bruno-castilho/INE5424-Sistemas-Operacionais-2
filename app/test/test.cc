#include <machine/display.h>
#include <time.h>
#include <synchronizer.h>
#include <process.h>

using namespace EPOS;

OStream cout;
Thread * phil[9];

int task(int n);

int main()
{
    for(int i = 0; i < 3; i++) {
        phil[i*3] = new Thread(Thread::Configuration(Thread::READY, Thread::LOW, Traits<Application>::STACK_SIZE), &task, i*3);
        phil[i*3+1] = new Thread(Thread::Configuration(Thread::READY, Thread::NORMAL, Traits<Application>::STACK_SIZE), &task, i*3+1);
        phil[i*3+2] = new Thread(Thread::Configuration(Thread::READY, Thread::HIGH, Traits<Application>::STACK_SIZE), &task, i*3+2);

    }


    for(int i = 0; i < 3; i++) {
        phil[i*3] ->join();
        phil[i*3+1] ->join();
        phil[i*3+2] ->join();
    }

    return 0;
}


int task(int n){
    cout << "Task running: " << n  << endl;
    Alarm::delay(10000000);

    cout << "Task finished: " <<  n  << endl;

    return 0;
}
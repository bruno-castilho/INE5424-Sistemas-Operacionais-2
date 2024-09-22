#include <machine/display.h>
#include <time.h>
#include <synchronizer.h>
#include <process.h>
#include <system/types.h>

using namespace EPOS;

OStream cout;
Clock clock;
Thread * phil[5];


int task(int n, int p);

int main()
{
    phil[0] = new Thread(
        Thread::Configuration(
            Thread::READY, 
            Thread::LOW, 
            10000000, 
            1000000, 
            Traits<Application>::STACK_SIZE
        ), 
        &task, 
        1,
        1000000
    );

    phil[1] = new Thread(
        Thread::Configuration(
            Thread::READY, 
            Thread::LOW, 
            15000000, 
            1500000, 
            Traits<Application>::STACK_SIZE
        ), 
        &task, 
        2,
        1500000
    );

    phil[2] = new Thread(
        Thread::Configuration(
            Thread::READY, 
            Thread::LOW, 
            13000000, 
            2000000, 
            Traits<Application>::STACK_SIZE
        ), 
        &task, 
        3,
        2000000
    );

    phil[3] = new Thread(
        Thread::Configuration(
            Thread::READY, 
            Thread::LOW, 
            1000000, 
            3000000, 
            Traits<Application>::STACK_SIZE
        ), 
        &task, 
        4,
        3000000
    );

    phil[4] = new Thread(
        Thread::Configuration(
            Thread::READY, 
            Thread::LOW, 
            100500000, 
            5000000, 
            Traits<Application>::STACK_SIZE
        ), 
        &task, 
        5,
        5000000
    );


    for(int i = 0; i < 5; i++) {
        phil[i] ->join();

    }



}


int task(int n, int p){
    for(int i = 0; i < 4; i++){
        cout << "Task running: " << n  << endl;
        Alarm::delay(p);
    }
        
    cout << "Task finished: " <<  n  << endl;

    return 0;
}


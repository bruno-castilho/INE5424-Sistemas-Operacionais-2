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

inline void exec(char c, int i = 0)
{
    Milisecond elapsed = chrono.read() / 1000;

    write.p();
    cout << "\n" << elapsed << " " << c << "-" << i << endl
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
        cout << "{" << cpu << ": " << Periodic_Thread::get_branch_misprediction_per_second(cpu) << "}" << endl;
    }

    write.v();
}


int main()
{
    cout << "Periodic Thread Component Test" << endl;

    cout << "\nCallibrating the duration of the base execution loop: ";
    callibrate();
    cout << base_loop_count << " iterations per ms!" << endl;


    cout << "\nThreads will now be created and I'll wait for them to finish..." << endl;


    // p,d,c,act,t
    thread_a = new Periodic_Thread(RTConf(period_a * 1000, period_a * 1000, 0, 0, iterations), &func_a);
    thread_b = new Periodic_Thread(RTConf(period_b * 1000, period_b * 1000, 0, 0, iterations), &func_b);
    thread_c = new Periodic_Thread(RTConf(period_c * 1000, period_c * 1000, 0, 0, iterations), &func_c);
    thread_d = new Periodic_Thread(RTConf(period_d * 1000, period_d * 1000, 0, 0, iterations), &func_d);
    thread_e = new Periodic_Thread(RTConf(period_e * 1000, period_e * 1000, 0, 0, iterations), &func_e);
    thread_f = new Periodic_Thread(RTConf(period_f * 1000, period_f * 1000, 0, 0, iterations), &func_f);


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

    cout << Periodic_Thread::get_changes_count() << " threads were replaced." << endl;
         
    cout << "I'm also done, bye!" << endl;

    return 0;
}





void merge(int arr[], int l, int m, int r) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    // Create temp arrays
    int L[n1], R[n2];

    // Copy data to temp arrays L[] and R[]
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    // Merge the temp arrays back into arr[l..r
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        }
        else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    // Copy the remaining elements of L[],
    // if there are any
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    // Copy the remaining elements of R[],
    // if there are any
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

void mergeSort(int arr[], int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;

        // Sort first and second halves
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);

        merge(arr, l, m, r);
    }
}

int binarySearch(int arr[], int low, int high, int x) {
    if (high >= low) {
        int mid = low + (high - low) / 2;

        if (arr[mid] == x)
            return mid;

        if (arr[mid] > x)
            return binarySearch(arr, low, mid - 1, x);

        return binarySearch(arr, mid + 1, high, x);
    }

    return -1;
}




int func_a()
{
    exec('A');

    do {
        exec('a', 1);

        int* array = new int[2000];
        array[0] = 3;
        for (int i = 1; i < 2000; ++i) {
            array[i] = ((array[i-1] * i * 39) % 27835);
        }

        mergeSort(array, 0, 1999);
        binarySearch(array, 0, 1999, 371);

        delete[] array;

        exec('a', 2);

    } while (Periodic_Thread::wait_next());
    exec('A');

    return 'A';
}

int func_b()
{
    exec('B');

    do {
        exec('b', 1);

        int* array = new int[2500];
        array[0] = 8;
        for (int i = 1; i < 2499; ++i) {
            array[i] = ((array[i-1] * i * 48) % 45637);
        }

        mergeSort(array, 0, 2499);
        binarySearch(array, 0, 2499, 835);

        delete[] array;

        exec('b', 2);
    } while (Periodic_Thread::wait_next());

    exec('B');

    return 'B';
}

int func_c()
{
    exec('C');

    do {
        exec('c', 1);

        int* array = new int[800];
        array[0] = 5;
        for (int i = 1; i < 799; ++i) {
            array[i] = ((array[i-1] * i * 75) % 45632);
        }

        mergeSort(array, 0, 799);
        binarySearch(array, 0, 799, 391);

        delete[] array;

        exec('c', 2);
    } while (Periodic_Thread::wait_next());

    exec('C');

    return 'C';
}

int func_d()
{
    exec('D');

    do {
        exec('d', 1);

        int* array = new int[1400];
        array[0] = 3;
        for (int i = 1; i < 1399; ++i) {
            array[i] = ((array[i-1] * i * 45) % 74576);
        }

        mergeSort(array, 0, 1399);
        binarySearch(array, 0, 1399, 795);

        delete[] array;

        exec('d', 2);
    } while (Periodic_Thread::wait_next());

    exec('D');

    return 'D';
}

int func_e()
{
    exec('E');

    do {
        exec('e', 1);

        int* array = new int[3400];
        array[0] = 3;
        for (int i = 1; i < 3399; ++i) {
            array[i] = ((array[i-1] * i * 23) % 82454);
        }

        mergeSort(array, 0, 3399);
        binarySearch(array, 0, 3399, 543);

        delete[] array;

        exec('e', 2);
    } while (Periodic_Thread::wait_next());

    exec('E');

    return 'E';
}


int func_f()
{
    exec('F');

    do {
        exec('f', 1);

        int* array = new int[1000];
        array[0] = 3;
        for (int i = 1; i < 1000; ++i) {
            array[i] = ((array[i-1] * i * 17) % 3264);
        }

        mergeSort(array, 0, 999);
        binarySearch(array, 0, 999, 371);

        delete[] array;

        exec('f', 2);
    } while (Periodic_Thread::wait_next());

    exec('F');

    return 'F';
}
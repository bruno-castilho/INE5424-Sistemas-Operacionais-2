// EPOS Scheduler Test Program

#include <architecture.h>
#include <utility/ostream.h>


using namespace EPOS;

OStream cout;

int main()
{
    cout << "PMU Test (using a P-EDF application)\n" << endl;
    cout << "Using " << CPU::cores() << " CPUs" << endl;

    PMU::config(3, PMU_Event::INSTRUCTIONS_RETIRED);
    PMU::config(4, PMU_Event::UNHALTED_CYCLES);
    PMU::config(5, PMU_Event::CPU_CYCLES);

    // Reading PMU while in QEMU is only available with KVM enabled, thus, if this feature is not available
    // the execution will stop at the first PMU::read()
    // The following test is functional when running this application in a real machine (or enabling the KVM feature) 

    for(unsigned int j = 0; j < 5; j++) {
        cout << "start" << endl;

        for(unsigned int i = 0; i < PMU::CHANNELS; i++)
            PMU::start(i);
        for(unsigned int i = 0; i < PMU::CHANNELS; i++)
            cout << "PMU::Counter[" << i << "]=" << PMU::read(i) << endl;
        for(unsigned int i = 0; i < PMU::CHANNELS; i++)
            PMU::stop(i);
        for(unsigned int i = 0; i < PMU::CHANNELS; i++)
            cout << "PMU::Counter[" << i << "]=" << PMU::read(i) << endl;
        for(unsigned int i = 0; i < PMU::CHANNELS; i++)
            PMU::reset(i);
        for(unsigned int i = 0; i < PMU::CHANNELS; i++)
            cout << "PMU::Counter[" << i << "]=" << PMU::read(i) << endl;

        cout << "end" << endl;
    }

    return 0;
}

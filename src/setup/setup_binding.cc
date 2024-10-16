// EPOS SETUP Binding

#include <machine.h>

__BEGIN_SYS

OStream kout, kerr;

__END_SYS

extern "C" {
    __USING_SYS;

    // Libc legacy
    void _exit(int s) { db<Setup>(ERR) << "_exit(" << s << ") called!" << endl; for(;;); }
    void __exit() { _exit(-1); }
    void __cxa_pure_virtual() { db<void>(ERR) << "Pure Virtual method called!" << endl; }

    // Utility-related methods that differ from kernel and user space.
    // OStream
    void _print(const char * s) { Display::puts(s); }
    static volatile int _setup_print_lock = -1;
    void _print_preamble() {
        if(Traits<System>::multicore) {
            static char tag[] = "<0>: ";

            int me = CPU::id();
            int last = CPU::cas(_setup_print_lock, -1, me);
            for(int i = 0, owner = last; (i < 1000) && (owner != me); i++, owner = CPU::cas(_setup_print_lock, -1, me));
            if(last != me) {
                tag[1] = '0' + CPU::id();
                _print(tag);
            }
        }
    }
    void _print_trailler(bool error) {
        if(Traits<System>::multicore) {
            static char tag[] = " :<0>";

            if(_setup_print_lock != -1) {
                tag[3] = '0' + CPU::id();
                _print(tag);

                _setup_print_lock = -1;
            }
        }
        if(error)
            Machine::panic();
    }
}


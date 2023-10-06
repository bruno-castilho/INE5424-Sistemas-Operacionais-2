// EPOS RISC-V IC Mediator Declarations

#ifndef __riscv_ic_h
#define __riscv_ic_h

#include <architecture/cpu.h>
#include <machine/ic.h>
#include <system/memory_map.h>

__BEGIN_SYS

// Core Local Interrupter (CLINT)
class CLINT
{
private:
    typedef CPU::Reg Reg;
    typedef CPU::Reg32 Reg32;
    typedef CPU::Reg64 Reg64;
    typedef CPU::Phy_Addr Phy_Addr;
    typedef CPU::Log_Addr Log_Addr;

public:
    static const unsigned int IRQS = 16;

    // Interrupts ([M|S]CAUSE with interrupt = 1)
    enum : unsigned long {
        IRQ_USR_SOFT            = 0,
        IRQ_SUP_SOFT            = 1,
        IRQ_MAC_SOFT            = 3,
        IRQ_USR_TIMER           = 4,
        IRQ_SUP_TIMER           = 5,
        IRQ_MAC_TIMER           = 7,
        IRQ_USR_EXT             = 8,
        IRQ_SUP_EXT             = 9,
        IRQ_MAC_EXT             = 11,
        INTERRUPT               = 1UL << (Traits<CPU>::WORD_SIZE - 1),
        INT_MASK                = ~INTERRUPT

    };

    // Registers offsets from CLINT_BASE
    enum {                                // Description
        MSIP                    = 0x0000, // Per-HART 32-bit software interrupts (IPI) trigger; each HART is offset by 4 bytes from MSIP
        MTIMECMP                = 0x4000, // Per-HART 64-bit timer compare register; lower 32 bits stored first; each HART is offset by 8 bytes from MTIMECMP
        MTIME                   = 0xbff8, // Timer counter shared by all HARTs lower 32 bits stored first
    };

    // MTVEC modes
    enum Mode {
        DIRECT  = 0,
        INDEXED = 1
    };


public:
    static void mtvec(Mode mode, Phy_Addr base) {
    	Reg tmp = (base & -4UL) | (Reg(mode) & 0x3);
        ASM("csrw mtvec, %0" : : "r"(tmp) : "cc");
    }

    static Reg mtvec() {
        Reg value;
        ASM("csrr %0, mtvec" : "=r"(value) : : );
        return value;
    }

    static Reg64 mtime() { return *reinterpret_cast<Reg64 *>(Memory_Map::CLINT_BASE + MTIME); }
    static void  mtimecmp(Reg64 v) { *reinterpret_cast<Reg64 *>(Memory_Map::CLINT_BASE + MTIMECMP + (CPU::mhartid() * 8)) = v; }
};

// Platform Level Interrupt Controller (PLIC)
class PLIC // The current PLIC driver only operates in M-Mode, for any hart. S-Mode is not supported yet.
{
public:
    typedef CPU::Reg Reg;
    typedef CPU::Reg32 Reg32;
    typedef CPU::Reg64 Reg64;
    typedef CPU::Phy_Addr Phy_Addr;
    typedef CPU::Log_Addr Log_Addr;

    typedef unsigned int Ex_Interrupt_Id;

    // PUTS THIS IN TRAITS LATER: EACH PLIC IMPLEMENTATION HAS A DIFFERENT NUMBER OF EINTS. 1024 IS THE MAXIMUM SUPPORTED
    static const unsigned int EIRQS = Traits<PLIC>::EIRQS; // All possible External interrupts. 1024 = EINT0 (no interrupt) to EINT1023.

    enum {
        MIN_GLOBAL_THRESHOLD = 0,
        MAX_GLOBAL_THRESHOLD = 7  // masks all interrupts.
    };

    // Registers offsets from PLIC_CPU_BASE
    enum {                                  // Description
        PLIC_PENDING      = 0x001000,     // PLIC Interrupt Pending base offset.
        PLIC_INT_ENABLE   = 0x002000,     // PLIC Interrupt Enable base offset.
        PLIC_THRESHOLD      = 0x200000,     // PLIC Interrupt Priority Threshold Register (threshold)
        PLIC_CLAIM          = 0x200004,     // PLIC Claim/Complete Register (claim)
    };

    enum Mode {
        MACHINE = 0,
        SUPERVISOR = 1, // Not supported yet
    };

public:
    // Get the next available interrupt. This is the "claim" process.
    // The PLIC will automatically sort by priority and hand us the
    // ID of the interrupt.
    static Reg32 next_external_pending() {
        // The claim register is filled with the highest-priority, enabled interrupt.
        unsigned int claim_no = get_claim_reg();
        db<PLIC>(TRC)<<"PLIC::next(): claim_no=" << claim_no << endl;
        return claim_no;
    }

    // Complete a pending interrupt by id. The id should come
    // from the next() function above.
    static void complete_external(Ex_Interrupt_Id id) {
        db<PLIC>(TRC) << "PLIC::complete(id=" << id << ")";
        get_claim_reg() = id;
    }

    // Set the global threshold. The threshold can be a value [0..7].
    // The PLIC will mask any interrupts at or below the given threshold.
    // This means that a threshold of 7 will mask ALL interrupts and
    // a threshold of 0 will allow ALL interrupts.
    static void set_threshold(unsigned int tsh) {
        db<PLIC>(TRC) << "PLIC::set_threshold(tsh=" << tsh << ")";
        Ex_Interrupt_Id const actual_tsh = tsh & 7;
        get_threshold_reg() = actual_tsh;
    }

    // permits all interrupts with non-zero priority.
    static void permit_all_external() {
        db<PLIC>(TRC) << "PLIC::permit_all()";
        set_threshold(MIN_GLOBAL_THRESHOLD);
    }

    // masks all interrupts, no matter the priority.
    static void mask_all_external() {
        db<PLIC>(TRC) << "PLIC::mask_all()";
        set_threshold(MAX_GLOBAL_THRESHOLD);
    }

    // Enable a given interrupt ID.
    static void enable_external(Ex_Interrupt_Id id) {
        db<PLIC>(TRC) << "PLIC::enable(id=" << id << ")";
        Reg32 enables = get_enable_reg(id);
        int actualId = 1 << (id % 32);

        // The register is a 32-bit register, so that gives us enables
        // for interrupt 31 through 1 (0 is hardwired to 0).
        get_enable_reg(id) = enables | actualId;
    }

    // Disable a given interrupt ID.
    static void disable_external(Ex_Interrupt_Id id) {
        db<PLIC>(TRC) << "PLIC::disable(id=" << id << ")";
        Reg32 enables = get_enable_reg(id);
        int actualId = 1 << (id % 32);

        // Clear the bit corresponding to the interrupt id
        get_enable_reg(id) = (enables & ~actualId);
    }

    // Set a given interrupt priority to the given priority.
    static void set_priority(Ex_Interrupt_Id id, unsigned int priority) {
        db<PLIC>(TRC) << "PLIC::set_priority(id=" << id << ",priority=" << priority << ")";
        unsigned int actualPriority = priority & 7; // 7 is the max priority
        get_priority_reg(id) = actualPriority; // Set the priority
    }

private:
    static volatile Reg32 & get_priority_reg(unsigned int id) {
        db<PLIC>(TRC)<<"PLIC::get_priority_reg: id=" << id << endl;
        return reinterpret_cast<volatile Reg32 *>(Memory_Map::PLIC_CPU_BASE)[id];
    }

    static volatile Reg32 & get_threshold_reg() {
        Reg hartId = CPU::mhartid();
        db<PLIC>(TRC)<<"PLIC::get_threshold_reg: hartId=" << hartId << endl;
        if (hartId == 0) {
            return reinterpret_cast<volatile Reg32 *>(Memory_Map::PLIC_CPU_BASE)[PLIC_THRESHOLD/sizeof(Reg32)];
        }
        else {
            // + 0x1000 gets to Hart-1 threshold + 0x2000 * (n - 1) gets to Hart-n threshold (skipping S-Mode)
            return reinterpret_cast<volatile Reg32 *>(Memory_Map::PLIC_CPU_BASE)[(PLIC_THRESHOLD + 0x1000 + 0x2000 * (hartId - 1))/sizeof(Reg32)];
        }
    }

    static volatile Reg32 & get_claim_reg() {
        Reg hartId = CPU::mhartid();
        db<PLIC>(TRC)<<"PLIC::get_claim_reg: hartId=" << hartId << endl;
        if (hartId == 0) {
            return reinterpret_cast<volatile Reg32 *>(Memory_Map::PLIC_CPU_BASE)[PLIC_CLAIM/sizeof(Reg32)];
        }
        else {
            // + 0x1000 gets to Hart-1 claim + 0x2000 * (n - 1) gets to Hart-n claim (skipping S-Mode)
            return reinterpret_cast<volatile Reg32 *>(Memory_Map::PLIC_CPU_BASE)[((PLIC_CLAIM + 0x1000) + 0x2000 * (hartId - 1))/sizeof(Reg32)];
        }
    }

    static bool is_pending(Ex_Interrupt_Id id) {
        // 32 registers with 32 bits for enable each. 0th bit is hardwired to 0.
        Reg32 pendingReg = *reinterpret_cast<Reg32 *>(Memory_Map::PLIC_CPU_BASE + (PLIC_PENDING/sizeof(Reg32)) + (id / 32));

        int actualId = 1 << (id % 32);

        return ((pendingReg & actualId) != 0);
    }

    static volatile Reg32 & get_enable_reg(Ex_Interrupt_Id id) {
        unsigned int hartId = CPU::mhartid();
        db<PLIC>(TRC)<<"PLIC::get_enable_reg: hartId=" << hartId << endl;
        // + 32*(hartId) gets to the correct hart's enable base address
        // + Math::floor(id/32) gets to the correct register within the hart's enable base address
        return reinterpret_cast<volatile Reg32 *>(Memory_Map::PLIC_CPU_BASE)[(PLIC_INT_ENABLE/sizeof(Reg32)) + 32*(hartId) + (id / 32)];
    }
};

class IC: private IC_Common, private CLINT, private PLIC
{
    friend class Setup;
    friend class Machine;

private:
    typedef CPU::Reg Reg;

public:
    static const unsigned int EXCS = CPU::EXCEPTIONS;
    static const unsigned int IRQS = CLINT::IRQS;
    static const unsigned int EIRQS = PLIC::EIRQS;
    static const unsigned int INTS = EXCS + IRQS + EIRQS;

    using IC_Common::Interrupt_Id;
    using IC_Common::Interrupt_Handler;

    enum {
        INT_SYS_TIMER = EXCS + IRQ_MAC_TIMER,
        INT_GIGABIT_ETH = EXCS + IRQS + Traits<PLIC>::INT_GIGABIT_ETH
        // Place supported local and external interrupts here
    };

public:
    IC() {}

    static Interrupt_Handler int_vector(Interrupt_Id i) {
        assert(i < INTS);
        return _int_vector[i];
    }

    static void int_vector(Interrupt_Id i, const Interrupt_Handler & h) {
        db<IC>(TRC) << "IC::int_vector(int=" << i << ",h=" << reinterpret_cast<void *>(h) <<")" << endl;
        assert(i < INTS);
        _int_vector[i] = h;
    }

    static void enable() {
        db<IC>(TRC) << "IC::enable()" << endl;
        CPU::mie(CPU::MSI | CPU::MTI | CPU::MEI);
    }

    static void enable(Interrupt_Id i) {
        db<IC>(TRC) << "IC::enable(int=" << i << ")" << endl;
        assert(i < INTS);
        enable();
        // TODO: this should handle individual INTs and also be done at PLIC

        // Implement CLINT enable here

        if (i >= EXCS + IRQS) { // External interrupts
            db<IC>(TRC) << "IC::enable(int=" << int2eirq(i) << ")" << endl;
            enable_external(int2eirq(i));
        }
    }

    static void disable() {
        db<IC>(TRC) << "IC::disable()" << endl;
        CPU::miec(CPU::MSI | CPU::MTI | CPU::MEI);
    }

    static void disable(Interrupt_Id i) {
        db<IC>(TRC) << "IC::disable(int=" << i << ")" << endl;
        assert(i < INTS);
        disable();
        // TODO: this should handle individual INTs and also be done at PLIC

        if (i >= EXCS + IRQS) { // External interrupts
            disable_external(int2eirq(i));
        }
    }

    static void set_external_priority(Interrupt_Id id, unsigned int priority) {
        id = int2eirq(id);
        set_priority(id, priority);
    }

    static Interrupt_Id int_id() {
        // Id is retrieved from [m|s]cause even if mip has the equivalent bit up, because only [m|s]cause can tell if it is an interrupt or an exception
        Reg id = CPU::mcause();
        if(id & INTERRUPT)
            return irq2int(id & INT_MASK);
        else
            return (id & INT_MASK);
    }

    static int irq2int(int i) { return i + EXCS; }
    static int int2irq(int i) { return i - EXCS; }

    static int eirq2int(unsigned int i) { return i + EXCS + IRQS; }
    static int int2eirq(unsigned int i) { return i - EXCS - IRQS; }

private:
    static void dispatch();

    // Logical handlers
    static void int_not(Interrupt_Id i);
    static void exception(Interrupt_Id i);
    static void external();

    // Physical handler
    static void entry() __attribute((naked, aligned(4)));

    static void init();

private:
    static Interrupt_Handler _int_vector[INTS];
};

__END_SYS

#endif

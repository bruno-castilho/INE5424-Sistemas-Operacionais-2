// EPOS RISC-V VisionFive 2 (GEM) Ethernet NIC Mediator Declarations

#ifndef __riscv_gem_h
#define __riscv_gem_h

#include <architecture.h>
#include <network/ethernet.h>
#include <utility/convert.h>
#include <machine/ic.h>

__BEGIN_SYS

class GEM {
protected:
  typedef CPU::Reg8 Reg8;
  typedef CPU::Reg16 Reg16;
  typedef CPU::Reg32 Reg32;
  typedef CPU::Reg64 Reg64;
  typedef CPU::Log_Addr Log_Addr;
  typedef CPU::Phy_Addr Phy_Addr;
  // typedef CPU::IO_Port IO_Port; // Not sure if used for RISC V?
  // typedef CPU::IO_Irq IO_Irq; // Not sure if used for RISC V?
  typedef MMU::DMA_Buffer DMA_Buffer;
  typedef Ethernet::Address MAC_Address;

public:
  // Register offsets
  enum {
    NWCTRL = 0x000,      /**< Network Control reg */
    NWCFG = 0x004,       /**< Network Config reg */
    NWSR = 0x008,        /**< Network Status reg */
    DMACFG = 0x010,       /**< DMA Config reg */
    TXSTATUS = 0x014,    /**< TX Status reg */
    RXQBASE = 0x018,     /**< RX Q Base address reg */
    TXQBASE = 0x01C,     /**< TX Q Base address reg */
    RXSTATUS = 0x020,    /**< RX Status reg */
    ISR = 0x024,         /**< Interrupt Status reg */
    INT_ENR = 0x028,         /**< Interrupt Enable reg */
    IDR = 0x02C,         /**< Interrupt Disable reg */
    IMR = 0x030,         /**< Interrupt Mask reg */
    SPADDR1L = 0x088,    /**< Specific1 addr low reg */
    SPADDR1H = 0x08C,    /**< Specific1 addr high reg */
  };

  // Network Control Register bits
  enum {
    TXSTART = 1 << 9,          /**< Transmit start */
    CLEAR_STATS_REGS = 1 << 5, /**< Clear stats bit */
    CTRL_MGMT_PORT_EN = 1 << 4, /**< Control management port enable */
    TX_EN = 1 << 3,            /**< Transmit enable */
    RX_EN = 1 << 2,            /**< Receive enable */
    LOCALLOOP = 1 << 1,        /**< Local loopback */
  };

  // Network Configuration Register bits
  enum {
    FULL_DUPLEX = 1 << 1, /**< Full duplex */
    PROMISC = 1 << 4,     /**< Promiscuous mode */
    STRIP_FCS = 1 << 17,     /**< Strip FCS field */
    _32_DBUS_WIDTH_SIZE = 0 << 21, /**< 32 bits size */
    IGNORE_FCS = 1 << 26,
  };

  /* Transmit Status Register bits*/
  enum {
    TX_STAT_COMPLETE = 1 << 5,
    TX_STAT_USED_BIT_READ = 1 << 0,
    TX_STAT_ALL = 0x1ff,
  };

  enum {
    RX_STAT_OVERRUN = 1 << 2,
    RX_STAT_FRAME_RECD = 1 << 1,
    RX_STAT_ALL = 0x0f,
  };

  // Interrupt register bits
  enum {
    INTR_RX_OVERRUN = 1 << 10,
    INTR_TX_COMPLETE = 1 << 7,
    INTR_TX_CORRUPT_AHB_ERR = 1 << 6,
    INTR_TX_USED_READ = 1 << 3,
    INTR_RX_USED_READ = 1 << 2,
    INTR_RX_COMPLETE = 1 << 1,
    INT_ALL = 0x7FFFEFF
  };

  // Transmit and Receive Descriptors (in the Ring Buffers)
  struct Desc {
    volatile Reg32 addr;
    volatile Reg32 ctrl;
  };

  // Receive Descriptor
  struct Rx_Desc : public Desc {
    enum {
      OWN = 1 << 0,
      WRAP = 1 << 1,
      EOF = 1 << 15,
      SOF = 1 << 14,
      SIZE_MASK = 0x3fff
    };

    void update_size(unsigned int size) {
      ctrl = (ctrl & ~SIZE_MASK) | (size & SIZE_MASK);
    }

    friend Debug &operator<<(Debug &db, const Rx_Desc &d) {
      db << "{" << hex << d.addr << dec << ","
         << (d.ctrl & SIZE_MASK) << "," << hex << d.ctrl << dec << "}";
      return db;
    }
  };

  // Transmit Descriptor
  struct Tx_Desc : public Desc {
    enum {
      OWN = 1U << 31,
      WRAP = 1 << 30,
      LAST_BUF = 1 << 15,
      SIZE_MASK = 0x1fff
    };

    inline void update_size(unsigned int size) {
      ctrl = (ctrl & ~SIZE_MASK) | (size & SIZE_MASK);
    }

    friend Debug &operator<<(Debug &db, const Tx_Desc &d) {
      db << "{" << hex << d.addr << dec << ","
         << (d.ctrl & SIZE_MASK) << "," << hex << d.ctrl << dec << "}";
      return db;
    }
  };

  static volatile Reg32 &reg(unsigned int o) {
    return reinterpret_cast<volatile Reg32 *>(
        Memory_Map::ETH_BASE)[o / sizeof(Reg32)];
  }

  static Reg32 dma_cfg_rx_size(unsigned long size) {
    return (size / 64) << 16;
  }
};

class SiFive_U_NIC : public NIC<Ethernet>, private GEM {

  friend class Machine_Common;

private:
  typedef IC_Common::Interrupt_Id Interrupt_Id;

  // Mode
  static const bool promiscuous = Traits<SiFive_U_NIC>::promiscuous;

  // Transmit and Receive Ring sizes
  static const unsigned int UNITS = Traits<SiFive_U_NIC>::UNITS;
  static const unsigned int TX_BUFS = Traits<SiFive_U_NIC>::SEND_BUFFERS;
  static const unsigned int RX_BUFS = Traits<SiFive_U_NIC>::RECEIVE_BUFFERS;

  // Size of the DMA Buffer that will host the ring buffers
  static const unsigned int DMA_BUFFER_SIZE =
      RX_BUFS * ((sizeof(Rx_Desc) + 15) & ~15U) +
      TX_BUFS * ((sizeof(Tx_Desc) + 15) & ~15U) +
      RX_BUFS * ((sizeof(Buffer) + 15) & ~15U) +
      TX_BUFS * ((sizeof(Buffer) + 15) & ~15U);

protected:
  SiFive_U_NIC(unsigned int unit, DMA_Buffer *dma_buf);

public:
  ~SiFive_U_NIC();
  int send(const Address &dst, const Protocol &prot, const void *data,
           unsigned int size);
  int receive(Address *src, Protocol *prot, void *data, unsigned int size);

  Buffer *alloc(const Address &dst, const Protocol &prot, unsigned int once,
                unsigned int always, unsigned int payload);
  int send(Buffer *buf);
  void free(Buffer *buf);

  const Address &address() { return _configuration.address; }
  void address(const Address &address) {
    _configuration.address = address;
    _configuration.selector = Configuration::ADDRESS;
    reconfigure(&_configuration);
  }

  bool reconfigure(const Configuration *c);
  const Configuration &configuration() { return _configuration; }

  const Statistics &statistics() {
    _statistics.time_stamp = TSC::time_stamp();
    return _statistics;
  }

  virtual void attach(Observer *o, const Protocol &p) {
    db<SiFive_U_NIC>(TRC) << "GEM::attach(p=" << p << ")" << endl;
    NIC<Ethernet>::attach(o, p);
    reg(NWCTRL) = reg(NWCTRL) | RX_EN; // enable receive int
  }

  virtual void detach(Observer *o, const Protocol &p) {
    NIC<Ethernet>::detach(o, p);
    if (!observers()) {
      reg(NWCTRL) = reg(NWCTRL) & ~RX_EN; // disable receive int
    };
  }

  static SiFive_U_NIC *get(unsigned int unit = 0) { return get_by_unit(unit); }

private:
  void receive();
  void configure();
  void handle_int();

  static void int_handler(Interrupt_Id interrupt);

  static void init(unsigned int unit);

  static SiFive_U_NIC *get_by_unit(unsigned int unit) {
    return _device;
  }

private:
  Configuration _configuration;
  Statistics _statistics;

  DMA_Buffer *_dma_buf;

  long _rx_cur;
  Rx_Desc *_rx_ring;
  Phy_Addr _rx_ring_phy;

  long _tx_cur;
  Tx_Desc *_tx_ring;
  Phy_Addr _tx_ring_phy;

  Buffer *_rx_buffer[RX_BUFS];
  Buffer *_tx_buffer[TX_BUFS];

  static SiFive_U_NIC* _device;
};

__END_SYS

#endif
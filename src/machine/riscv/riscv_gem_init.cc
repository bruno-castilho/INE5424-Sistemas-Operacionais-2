#include <machine/machine.h>
#include <machine/riscv/riscv_gem.h>
#include <system.h>

__BEGIN_SYS

SiFive_U_NIC::SiFive_U_NIC(unsigned int unit, DMA_Buffer *dma_buf)
{
  db<SiFive_U_NIC>(TRC) << "SiFive_U_NIC(dma=" << dma_buf << ")" << endl;

  _configuration.unit = unit;
  _configuration.timer_frequency = TSC::frequency();
  _configuration.timer_accuracy = TSC::accuracy() / 1000; // PPB -> PPM
  if (!_configuration.timer_accuracy)
    _configuration.timer_accuracy = 1;

  _dma_buf = dma_buf;

  // Initialize controller
  reg(NWCTRL) = 0;
  reg(NWCTRL) = CLEAR_STATS_REGS;
  reg(NWCFG) = _32_DBUS_WIDTH_SIZE; // 32-bit data bus
  reg(TXSTATUS) = TX_STAT_ALL;
  reg(RXSTATUS) = RX_STAT_ALL;
  reg(IDR) = INT_ALL;
  reg(TXQBASE) = 0;
  reg(RXQBASE) = 0;

  // Distribute the DMA_Buffer allocated by init()
  Log_Addr log = _dma_buf->log_address();
  Phy_Addr phy = _dma_buf->phy_address();

  // Rx_Desc Ring
  _rx_cur = 0;
  _rx_ring = log;
  _rx_ring_phy = phy;
  log += RX_BUFS * align64(sizeof(Rx_Desc));
  phy += RX_BUFS * align64(sizeof(Rx_Desc));

  // Tx_Desc Ring
  _tx_cur = 0;
  _tx_ring = log;
  _tx_ring_phy = phy;
  log += TX_BUFS * align64(sizeof(Tx_Desc));
  phy += TX_BUFS * align64(sizeof(Tx_Desc));

  // Rx_Buffer Ring
  for (unsigned int i = 0; i < RX_BUFS; i++)
  {
    _rx_buffer[i] = new (log) Buffer(this, &_rx_ring[i]);
    //_rx_ring[i].update_size(sizeof(Frame));
    _rx_ring[i].addr = phy;    // Keep bits [1-0] from the existing value, and combine with bits [31-2] from buffer addr, manual says do this
    _rx_ring[i].addr &= ~Rx_Desc::OWN; // Owned by NIC
    _rx_ring[i].ctrl = 0;

    log += align64(sizeof(Buffer));
    phy += align64(sizeof(Buffer));
  }
  _rx_ring[RX_BUFS - 1].addr |= Rx_Desc::WRAP; // Mark the last descriptor in the buffer descriptor list with the wrap bit, (bit [1] in word [0]) set.

  // Tx_Buffer Ring
  for (unsigned int i = 0; i < TX_BUFS; i++)
  {
    _tx_buffer[i] = new (log) Buffer(this, &_tx_ring[i]);
    _tx_ring[i].addr = phy;
    //_tx_ring[i].update_size(0); // Clear size
    _tx_ring[i].ctrl |= Tx_Desc::OWN; // Owned by host

    log += align64(sizeof(Buffer));
    phy += align64(sizeof(Buffer));
  }
  _tx_ring[TX_BUFS - 1].ctrl |= Tx_Desc::WRAP; // Mark the last descriptor in the list with the wrap bit. Set bit [30] in word [1] to 1

  configure();
}

void SiFive_U_NIC::init(unsigned int unit)
{
  db<Init, SiFive_U_NIC>(TRC) << "SiFive_U_NIC::init()" << endl;

  // Allocate a DMA Buffer for rx and tx rings
  DMA_Buffer *dma_buf = new (SYSTEM) DMA_Buffer(DMA_BUFFER_SIZE);

  // Initialize the device
  SiFive_U_NIC *dev = new (SYSTEM) SiFive_U_NIC(unit, dma_buf);

  // Register the device
  SiFive_U_NIC::_device = dev;

  // Install interrupt handler
  IC::int_vector(IC::INT_GIGABIT_ETH, &int_handler);

  // Enable interrupts for device
  IC::enable(IC::INT_GIGABIT_ETH);

  IC::set_external_priority(IC::INT_GIGABIT_ETH, 7);
}

__END_SYS
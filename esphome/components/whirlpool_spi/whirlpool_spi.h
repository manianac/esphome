#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

#include "driver/spi_slave.h"

namespace esphome {
namespace whirlpool_spi {

class WhirlpoolSPI : public Component {
 public:
  WhirlpoolSPI() = default;

  void setup() override;
  void dump_config() override;
  void loop() override;

  void set_mosi_pin(GPIOPin *pin) { mosi_pin_ = pin; }
  void set_clk_pin(GPIOPin *pin) { clk_pin_ = pin; }
  void set_miso_pin(GPIOPin *pin) { miso_pin_ = pin; }
  void set_cs_pin(GPIOPin *pin) { cs_pin_ = pin; }
  void set_buffer_size(size_t size) { buffer_size_ = size; }

 private:
  GPIOPin *mosi_pin_{nullptr};
  GPIOPin *clk_pin_{nullptr};
  GPIOPin *miso_pin_{nullptr};
  GPIOPin *cs_pin_{nullptr};
  size_t buffer_size_{64};

  // SPI slave transaction buffer
  uint8_t *rx_buffer_{nullptr};
  uint8_t *tx_buffer_{nullptr};
  spi_slave_transaction_t trans_{};
  bool trans_queued_{false};
};

}  // namespace whirlpool_spi
}  // namespace esphome

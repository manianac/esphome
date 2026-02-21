#include "whirlpool_spi.h"

#include "esphome/core/log.h"
#include "esphome/core/gpio.h"
#include "driver/gpio.h"

namespace esphome {
namespace whirlpool_spi {

static const char *const TAG = "whirlpool_spi";

void WhirlpoolSPI::setup() {
  ESP_LOGCONFIG(TAG, "Setting up WhirlpoolSPI...");

  // Allocate transaction buffers
  rx_buffer_ = new uint8_t[buffer_size_];
  tx_buffer_ = new uint8_t[buffer_size_];

  if (!rx_buffer_ || !tx_buffer_) {
    ESP_LOGE(TAG, "Failed to allocate transaction buffers");
    mark_failed();
    return;
  }

  // Initialize buffers to zero
  memset(rx_buffer_, 0, buffer_size_);
  memset(tx_buffer_, 0, buffer_size_);

  // Get GPIO pin numbers
  int mosi_num = 0;
  int clk_num = 0;
  int miso_num = -1;
  int cs_num = -1;

  // Extract GPIO numbers from pins (cast to InternalGPIOPin to access get_pin())
  if (mosi_pin_ != nullptr) {
    auto *internal_pin = static_cast<InternalGPIOPin *>(mosi_pin_);
    mosi_num = internal_pin->get_pin();
  } else {
    ESP_LOGE(TAG, "MOSI pin not configured");
    mark_failed();
    return;
  }

  if (clk_pin_ != nullptr) {
    auto *internal_pin = static_cast<InternalGPIOPin *>(clk_pin_);
    clk_num = internal_pin->get_pin();
  } else {
    ESP_LOGE(TAG, "CLK pin not configured");
    mark_failed();
    return;
  }

  if (miso_pin_ != nullptr) {
    auto *internal_pin = static_cast<InternalGPIOPin *>(miso_pin_);
    miso_num = internal_pin->get_pin();
  }

  if (cs_pin_ != nullptr) {
    auto *internal_pin = static_cast<InternalGPIOPin *>(cs_pin_);
    cs_num = internal_pin->get_pin();
  } else {
    ESP_LOGE(TAG, "CS pin not configured");
    mark_failed();
    return;
  }

  // Log pin configuration
  ESP_LOGCONFIG(TAG, "  MOSI Pin: GPIO%d", mosi_num);
  ESP_LOGCONFIG(TAG, "  CLK Pin: GPIO%d", clk_num);
  if (miso_pin_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  MISO Pin: GPIO%d", miso_num);
  }
  ESP_LOGCONFIG(TAG, "  CS Pin: GPIO%d", cs_num);
  ESP_LOGCONFIG(TAG, "  Buffer Size: %u bytes", buffer_size_);

  // Configure pull-up resistors on MOSI and CLK pins
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << static_cast<uint32_t>(mosi_num)) | (1ULL << static_cast<uint32_t>(clk_num)),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };

  esp_err_t ret = gpio_config(&io_conf);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure pull-up resistors: %s", esp_err_to_name(ret));
    mark_failed();
    return;
  }

  // Initialize SPI slave bus configuration
  spi_bus_config_t buscfg = {
      .mosi_io_num = mosi_num,
      .miso_io_num = miso_num,
      .sclk_io_num = clk_num,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = static_cast<int>(buffer_size_),
      .flags = 0,
      .intr_flags = 0,
  };

  // Initialize SPI slave configuration
  spi_slave_interface_config_t slvcfg = {
      .spics_io_num = cs_num,
      .flags = 0,
      .queue_size = 3,
      .mode = 0,  // SPI mode 0
  };

  // Initialize SPI slave
  ret = spi_slave_initialize(SPI2_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize SPI slave: %s", esp_err_to_name(ret));
    mark_failed();
    return;
  }

  ESP_LOGI(TAG, "SPI slave initialized on SPI2_HOST");

  // Prepare initial transaction
  trans_.length = buffer_size_ * 8;  // Length in bits
  trans_.rx_buffer = rx_buffer_;
  trans_.tx_buffer = tx_buffer_;

  // Queue initial transaction
  ret = spi_slave_queue_trans(SPI2_HOST, &trans_, portMAX_DELAY);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to queue initial transaction: %s", esp_err_to_name(ret));
    mark_failed();
    return;
  }

  trans_queued_ = true;
  ESP_LOGI(TAG, "Initial transaction queued");
}

void WhirlpoolSPI::dump_config() {
  ESP_LOGCONFIG(TAG, "Whirlpool SPI:");
  LOG_PIN("  MOSI Pin: ", this->mosi_pin_);
  LOG_PIN("  CLK Pin: ", this->clk_pin_);
  LOG_PIN("  MISO Pin: ", this->miso_pin_);
  LOG_PIN("  CS Pin: ", this->cs_pin_);
  ESP_LOGCONFIG(TAG, "  Buffer Size: %u", static_cast<unsigned>(this->buffer_size_));
}

void WhirlpoolSPI::loop() {
  if (!trans_queued_) {
    return;
  }

  spi_slave_transaction_t *rtrans = nullptr;

  // Non-blocking check for completed transaction
  esp_err_t ret = spi_slave_get_trans_result(SPI2_HOST, &rtrans, 0);

  if (ret == ESP_OK && rtrans != nullptr) {
    // Transaction completed, process received data
    size_t bytes_received = rtrans->trans_len / 8;  // Convert bits to bytes

    if (bytes_received > 0) {
      ESP_LOGD(TAG, "Received %u bytes:", bytes_received);

      // Log hex dump of received data
      for (size_t i = 0; i < bytes_received; i++) {
        if (i % 16 == 0) {
          if (i > 0) {
            ESP_LOGD(TAG, "");
          }
          ESP_LOGD(TAG, "  %04zx: ", i);
        }
        ESP_LOGD(TAG, "%02x ", rx_buffer_[i]);
      }
      ESP_LOGD(TAG, "");
    }

    // Clear the receive buffer for next transaction
    memset(rx_buffer_, 0, buffer_size_);

    // Re-queue the next transaction
    trans_.length = buffer_size_ * 8;
    trans_.rx_buffer = rx_buffer_;
    trans_.tx_buffer = tx_buffer_;

    ret = spi_slave_queue_trans(SPI2_HOST, &trans_, 0);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to re-queue transaction: %s", esp_err_to_name(ret));
      trans_queued_ = false;
      mark_failed();
      return;
    }
  } else if (ret == ESP_ERR_TIMEOUT) {
    ESP_LOGVV(TAG, "Waiting for SPI message...");
  } else {
    // Some error other than timeout occurred
    ESP_LOGW(TAG, "Error getting transaction result: %s", esp_err_to_name(ret));
  }
}

}  // namespace whirlpool_spi
}  // namespace esphome

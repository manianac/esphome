# WhirlpoolSPI Component

A custom ESPHome component for ESP32 that implements an SPI slave receiver using the ESP-IDF driver. This component uses `SPI2_HOST` to receive data as an SPI slave device.

## Features

- **SPI Slave Mode**: Operates as an SPI slave on `SPI2_HOST` using ESP-IDF's `spi_slave_initialize` driver
- **Non-blocking Operation**: Uses non-blocking `spi_slave_get_trans_result()` in the loop
- **Configurable Pins**: Support for MOSI, CLK (required), MISO, and CS (optional)
- **Pull-up Resistors**: Automatically configures pull-up resistors on MOSI and CLK pins
- **Flexible Buffer Size**: Configurable receive buffer size (default 64 bytes)
- **Hex Dump Logging**: Logs received data as hex dumps at debug level
- **Auto-requeuing**: Automatically re-queues the next transaction after processing

## Requirements

- **Platform**: ESP32 only
- **Framework**: ESP-IDF framework (Arduino framework not supported)

## Configuration

### Basic Configuration

```yaml
whirlpool_spi:
  mosi_pin: GPIO23
  clk_pin: GPIO18
  miso_pin: GPIO19
  cs_pin: GPIO5
  buffer_size: 64
```

### Required Pins

- **`mosi_pin`** (required): GPIO pin for Master Out Slave In (data from master to slave)
- **`clk_pin`** (required): GPIO pin for clock signal

### Optional Pins

- **`miso_pin`** (optional): GPIO pin for Master In Slave Out (data from slave to master). If not specified, no MISO line is used.
- **`cs_pin`** (optional): GPIO pin for Chip Select. If not specified, the slave is always active (no CS required).

### Parameters

- **`buffer_size`** (optional, default: 64): Size of the SPI transaction buffer in bytes. Must be between 1 and 4096.

## Pin Configuration Example

For an ESP32-DevKit with common SPI pinout:

```yaml
whirlpool_spi:
  mosi_pin: GPIO23   # SPI MOSI
  clk_pin: GPIO18    # SPI Clock
  miso_pin: GPIO19   # SPI MISO (optional)
  cs_pin: GPIO5      # SPI Chip Select (optional)
  buffer_size: 128
```

## How It Works

### Setup Phase

1. Allocates RX and TX buffers of the configured size
2. Extracts GPIO pin numbers from the pin objects
3. Configures pull-up resistors on MOSI and CLK pins for signal integrity
4. Initializes the SPI slave bus with the provided pin configuration
5. Initializes the SPI slave interface on `SPI2_HOST`
6. Queues the initial transaction and waits for data

### Loop Phase

1. Non-blocking check for completed SPI transactions (timeout = 0)
2. If a transaction completes:
   - Logs received bytes as a formatted hex dump
   - Clears the receive buffer
   - Re-queues the next transaction
3. If no transaction has completed or an error occurs, returns and tries again on the next loop cycle

## Logging

The component uses the `whirlpool_spi` tag for logging:

- **ERROR**: Critical failures (failed to allocate buffers, initialize SPI, etc.)
- **INFO**: Setup completion and initial transaction queuing
- **DEBUG**: Received data hex dumps and transaction details

Example debug output:
```
[DEBUG] whirlpool_spi: Received 32 bytes:
[DEBUG] whirlpool_spi:   0000: 48 65 6c 6c 6f 20 57 6f 72 6c 64 21 00 00 00 00
[DEBUG] whirlpool_spi:   0010: 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f 50
```

## SPI Slave Configuration

The component configures the SPI slave with the following settings:

- **SPI Mode**: 0 (CPOL=0, CPHA=0)
- **DMA**: Automatic DMA channel selection (`SPI_DMA_CH_AUTO`)
- **Queue Size**: 3 transactions
- **Max Transfer Size**: Based on configured buffer size

## Limitations

- **ESP32 Only**: This component is designed specifically for ESP32 and will not work on other platforms
- **ESP-IDF Framework Only**: Requires the ESP-IDF framework; Arduino framework is not supported
- **SPI2_HOST Only**: Currently uses SPI2_HOST; SPI3_HOST is not supported
- **One Instance**: Only one instance of this component can be used per device

## Error Handling

The component will mark itself as failed if:
- Memory allocation fails
- Pin configuration is missing (MOSI or CLK)
- GPIO pull-up configuration fails
- SPI slave initialization fails
- Transaction queuing fails

In any failure case, check the error logs for details.

## Example Full Configuration

```yaml
esphome:
  name: whirlpool_receiver
  friendly_name: "Whirlpool SPI Receiver"

esp32:
  board: esp32dev
  framework:
    type: esp-idf
    version: recommended

logger:
  level: DEBUG
  logs:
    whirlpool_spi: DEBUG

uart:
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 115200

whirlpool_spi:
  mosi_pin: GPIO23
  clk_pin: GPIO18
  miso_pin: GPIO19
  cs_pin: GPIO5
  buffer_size: 256
```

## Performance Notes

- The component uses non-blocking transactions in the main loop, ensuring it doesn't block other ESPHome tasks
- Transactions are automatically re-queued for continuous reception
- With the default 64-byte buffer, typical transaction times are microseconds on modern ESP32 devices
- Increasing `buffer_size` increases per-transaction data capacity but also RAM usage

## Troubleshooting

### No Data Received

1. Verify pin connections match your YAML configuration
2. Ensure the SPI master is sending data correctly
3. Check pull-up resistor configuration is enabled (automatic in this component)
4. Review debug logs for SPI initialization errors

### Intermittent Data Loss

1. Try increasing the queue size (currently hardcoded to 3)
2. Ensure other ESPHome components aren't blocking the loop
3. Check that buffer size is appropriate for your data rate

### Out of Memory

- Reduce `buffer_size` if allocating too much RAM
- Ensure other components aren't consuming excess memory

## Development

To add features or modify this component:

1. Component configuration is in `__init__.py`
2. C++ header definitions are in `whirlpool_spi.h`
3. C++ implementation is in `whirlpool_spi.cpp`

All three files must be updated for feature additions.

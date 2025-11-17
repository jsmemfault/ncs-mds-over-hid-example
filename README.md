# MDS over HID Application

Zephyr RTOS application implementing the Memfault Diagnostic Service (MDS) protocol over USB HID for nRF52840.

## Features

- USB HID interface for MDS protocol communication
- Memfault diagnostics data collection and streaming
- Configurable heartbeat metrics (10s for testing, 1h default for production)
- Support for all MDS protocol reports:
  - Supported Features (0x01)
  - Device Identifier (0x02)
  - Data URI (0x03)
  - Authorization (0x04)
  - Stream Control (0x05)
  - Stream Data (0x06)

## Hardware

- **Board**: nRF52840 DK (nrf52840dk/nrf52840)
- **USB**: VID 0x2fe3 (Zephyr Project), PID 0x0007

## Prerequisites

- Python 3.10 or newer
- [Zephyr SDK](https://docs.zephyrproject.org/latest/develop/toolchains/zephyr_sdk.html) installed
- nRF52840 DK hardware

## Setup

### 1. Clone the Repository

```bash
git clone <repository-url>
cd mds-over-hid-app
```

### 2. Create Python Virtual Environment

```bash
python3 -m venv .venv
source .venv/bin/activate
```

### 3. Install West

```bash
pip install west
```

### 4. Initialize West Workspace

This pulls Zephyr RTOS, nRF SDK, and all dependencies (this may take several minutes):

```bash
west init -l app
west update
```

### 5. Install Python Dependencies

```bash
pip install -r zephyr/scripts/requirements.txt
```

### 6. Configure Memfault

Copy the template configuration and add your Memfault project key:

```bash
cp app/prj.conf.template app/prj.conf
# Edit app/prj.conf and replace YOUR_PROJECT_KEY_HERE with your actual key
```

Get your project key from https://app.memfault.com/ → Settings → General

### 7. Build

```bash
cd app
west build -b nrf52840dk/nrf52840
```

### 8. Flash

Connect your nRF52840 DK via USB and flash:

```bash
west flash
```

## Configuration

Key configuration options in `app/prj.conf`:

- `CONFIG_MEMFAULT_NCS_PROJECT_KEY`: Your Memfault project key (required)
- `CONFIG_MEMFAULT_NCS_DEVICE_ID`: Unique device identifier
- `CONFIG_MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS`: Metrics collection interval (10s for testing)

## Protocol

This device implements the MDS protocol over USB HID. The host can:
1. Query device information via Feature Reports (IDs 1-4)
2. Enable/disable streaming via Stream Control (ID 5)
3. Receive diagnostic data chunks via Stream Data (ID 6)

## Development

The application uses Memfault SDK integration for NCS. Diagnostic data is automatically collected and queued for transmission when streaming is enabled by the host.

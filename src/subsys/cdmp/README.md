# CAN Device Management Protocol (CDMP)

**Version 1.0**  
**Date: December 27, 2024**

-----

## Table of Contents

1. [System Overview](#1-system-overview)
1. [Physical and Data Link Layer](#2-physical-and-data-link-layer)
1. [Addressing and Device Management](#3-addressing-and-device-management)
1. [Message Types and Formats](#4-message-types-and-formats)
1. [Bulk Data Transfer (ISO-TP Layer)](#5-bulk-data-transfer-iso-tp-layer)
1. [Protocol Status Machine](#6-protocol-status-machine)
1. [Error Handling and Reliability](#7-error-handling-and-reliability)
1. [Configuration Management](#8-configuration-management)
1. [Implementation Requirements](#9-implementation-requirements)
1. [Testing and Validation](#10-testing-and-validation)
1. [Protocol Versioning](#11-protocol-versioning)
1. [Security Considerations](#12-security-considerations)
1. [Documentation and Tooling](#13-documentation-and-tooling)
1. [Capability-Based Data Streaming](#14-capability-based-data-streaming)
1. [Appendix A: Message Type Summary](#appendix-a-message-type-summary)
1. [Appendix B: Example Message Flows](#appendix-b-example-message-flows)

-----

## 1. System Overview

### 1.1 Purpose

A lightweight management protocol operating on CAN bus alongside standard DBC-defined sensor messages, enabling device discovery, configuration, and status management for smart devices while maintaining backward compatibility with simple devices.

### 1.2 Scope

**In scope:**

- Device discovery
- ID assignment
- Configuration transfer
- Settings management
- Status synchronization
- Heartbeat monitoring
- Capability-based data streaming

**Out of scope:**

- Safety-critical real-time control
- Guaranteed deterministic timing
- Encryption/security features

### 1.3 Coexistence Model

- **Normal CAN traffic** (0x100-0x6FF): Sensor data streams, DBC-defined formats, processed by all devices
- **Management protocol** (Base + 0 to Base + 99): Device management, processed only by protocol-aware devices
- Simple devices ignore management CAN IDs and operate normally

-----

## 2. Physical and Data Link Layer

### 2.1 CAN Configuration

- **Bus speed**: 1 Mbps (configurable: 250 kbps, 500 kbps)
- **Frame format**: Standard CAN 2.0B (11-bit identifiers, 8-byte payload)
- **Optional**: CAN-FD support for higher throughput

### 2.2 CAN ID Allocation

**Base CAN ID:** Configurable (default 0x700), set during system configuration

**Management Protocol (Fixed Offsets from Base):**

```
Base + 0:    Management broadcasts (discovery, heartbeat, announcements)
Base + 1:    Status broadcasts (generic device status, time-slotted)
Base + 2:    Command requests (settings, actions)
Base + 3:    Command responses (ACKs, results)
Base + 4:    State change notifications
Base + 5:    State change responses
Base + 6:    ISO-TP request (bulk data transfer sender)
Base + 7:    ISO-TP response (bulk data transfer receiver, flow control)
Base + 8-19: Reserved for protocol expansion
```

**Capability-Specific Streams (Dynamic Registration):**

```
Base + 20-51: Capability stream CAN IDs (32 slots available)
```

Capability CAN IDs are assigned through a registration mechanism in the implementation library. Applications register capabilities with the protocol stack, which maps them to available CAN ID slots (Base + 20 through Base + 51).

**Application specific CAN IDs**

```
Base + 51-99: CAN IDs available for application specific uses (48 slots available)
```

Application specific CAN IDs are available for aplication specific uses.

**Registration Model:**
- Application defines capability (name, data format, update rate)
- Library assigns next available CAN ID from pool (Base + 20+)
- Capability ID and CAN ID mapping stored in device configuration
- Heartbeat capability flags indicate which capabilities are active

**Example Capability Registrations:**
```
Library provides registration API:
register_capability(capability_bit_number, data_format, update_rate)

Examples:
- Logging capability: register_capability(0, ...)  // Bit 0, Base+20
- GPS capability: register_capability(1, ...)      // Bit 1, Base+21
- Custom sensor: register_capability(10, ...)      // Bit 10, Base+30
```

Returns CAN ID as Base + 20 + capability_bit_number.

The protocol does not predefine specific capabilities or their CAN ID assignments. All capability definitions, data formats, and CAN ID mappings are application-specific and registered at initialization time. The protocol only provides the framework for capability registration, discovery, and time-slotted transmission.

**Application Messages (Outside Protocol Scope):**

```
0x100-0x6FF: Sensor data, control messages (DBC-defined)
```

**Note:** When base = 0x700 (default), management protocol uses 0x700-0x7FF, leaving 0x100-0x6FF free for application messages.

-----

## 3. Addressing and Device Management

### 3.1 Device Identification

- **Logical Device ID**: uint8_t (1-254), dynamically assigned during startup
  - 0: Reserved (invalid/unassigned)
  - 255: Reserved (broadcast address)
- **Unique Identifier**: 32-bit (serial number, MAC address, random number, or other source)
- **Device Type**: uint8_t enumeration (application-specific)

### 3.2 ID Assignment Protocol

#### 3.2.1 Discovery-Based Startup Sequence

1. **Power on**: Device initializes with no Device ID assigned
1. **Startup backoff**: Wait random(0-500ms) to stagger startup
   - Use true random source if available (ADC noise, etc.)
   - Fallback: pseudo-random seeded from UID
1. **Send Discovery Request**: Immediately broadcast on Base + 0
1. **Wait for responses**: 200ms timeout for Discovery Responses
1. **Retry if needed**: If no responses received, retry with backoff (100ms, 200ms, 500ms)
1. **Select ID**: After collecting responses (or 3 failed attempts), choose ID:
   - If responses received: Select lowest available ID (1-254), filling gaps
   - If no responses after 3 retries: Assign ID = 1 (first device on network)
1. **ID claim**: Broadcast on Base + 0 with unique identifier (32-bit)
1. **Conflict detection**: Wait 50ms for objections
1. **Confirmation**: Begin normal operation

**ID Selection Algorithm:**
- Build list of used IDs from Discovery Responses
- Select smallest unused ID starting from 1
- Example: If IDs 1, 2, 5, 7 are in use → select ID 3
- If no devices respond (empty network): Default to ID 1

**Retry Mechanism:**
- Attempt 1: Send Discovery Request, wait 200ms
- Attempt 2: Wait 100ms backoff, send Discovery Request, wait 200ms
- Attempt 3: Wait 200ms backoff, send Discovery Request, wait 200ms
- After 3 attempts with no responses: Assume empty network, claim ID 1

**Rationale:** Discovery-first approach provides complete network visibility before claiming an ID. This eliminates blind guessing, fills ID gaps efficiently, and ensures optimal ID allocation. The retry mechanism ensures robust operation even with temporary bus issues, while the ID=1 fallback handles the first-device case cleanly.

#### 3.2.2 ID Claim Message Format (CAN ID Base + 0)

```
Byte 0:    Message Type = 0x01 (ID_CLAIM)
Byte 1:    Claiming Device ID
Byte 2-5:  Unique Identifier (32-bit)
Byte 6:    Device Type
Byte 7:    Protocol Version
```

#### 3.2.3 Conflict Resolution

**Arbitration Loss:**
- CAN arbitration loss during ID_CLAIM SHALL be treated as implicit conflict
- Device detecting arbitration loss SHALL:
  1. Abort current claim attempt
  2. Wait random backoff (50-200ms)  
  3. Send Discovery Request to rescan network
  4. Select available ID (may be same or different)
  5. Retry ID_CLAIM

**Version Incompatibility:**
- If device receives ID_CLAIM with incompatible major protocol version:
  1. All existing ONLINE devices SHALL immediately enter VERSION_MISMATCH status
  1. Device with lowest ID SHALL send ID_CLAIM_RESPONSE with result 0x02
  1. Other devices SHALL NOT send ID_CLAIM_RESPONSE
  1. All existing devices SHALL cease all protocol transmissions (heartbeat, status, commands)
  1. All existing devices continue DBC-defined message operations
  1. Recovery requires reboot of all devices

#### 3.2.4 ID Claim Response (CAN ID Base + 0)

```
Byte 0:    Message Type = 0x02 (ID_CLAIM_RESPONSE)
Byte 1:    Responding Device ID
Byte 2:    Claiming Device ID (being contested)
Byte 3:    Result (0x00=Accept, 0x01=Reject/Conflict, 0x02=Version Incompatible)
Byte 4-7:  Reserved
```

**Result Codes:**

```
0x00 (Accept): ID claim accepted, no conflict
0x01 (Reject/Conflict): ID already in use, device should retry with different ID
0x02 (Version Incompatible): Protocol major version mismatch detected
```

**Behavior:**

If new device receives 0x02 response:

- Enter VERSION_MISMATCH status immediately
- Cease all protocol transmissions
- Continue DBC-defined message operations
- Require reboot to retry

**Response Arbitration:**

- For version incompatibility (0x02): Device with lowest ID sends response
- For ID conflicts (0x01): Device whose ID is being claimed sends response

### 3.3 Device Discovery

Any device can request network enumeration:

**Discovery Request (CAN ID Base + 0)**

```
Byte 0:    Message Type = 0x03 (DISCOVERY_REQUEST)
Byte 1-4:  Unique Identifier (32-bit)
Byte 5-7:  Reserved
```

**Discovery Response (CAN ID Base + 0, staggered 5-10ms per device)**

```
Byte 0:    Message Type = 0x04 (DISCOVERY_RESPONSE)
Byte 1:    Device ID
Byte 2-5:  Unique Identifier (32-bit)
Byte 6:    Device Type
Byte 7:    Reserved
```

### 3.4 Heartbeat

Each device sends periodic heartbeat to indicate liveness.

**Heartbeat Message (CAN ID Base + 0)**

```
Byte 0:    Message Type = 0x05 (HEARTBEAT)
Byte 1:    Device ID
Byte 2:    Health Status (0x00=OK, 0x01=Warning, 0x02=Error)
Byte 3:    Uptime Counter (rolls over at 255)
Byte 4-7:  Capability Flags (32 bits, LSB first)
```

**Note:** See section 14 for details on capability flags.

**Timing:**

- **Interval**: 1-5 seconds (configurable per device)
- **Timeout**: 3× heartbeat interval (missing 3 consecutive = presumed offline)
- **Purpose**: Liveness indication and capability advertisement

-----

## 4. Message Types and Formats

### 4.1 Status Broadcasts (CAN ID Base + 1)

**Purpose**: Periodic generic device status, non-critical data

**Message Format:**

```
Byte 0:    Device ID
Byte 1:    Sequence Number (0-255, rolls over)
Byte 2-7:  Status Data (application-specific)
```

**Timing:**

- **Base frequency**: 1 Hz (configurable 0.1-10 Hz)
- **Time-slotting**: Offset = Device_ID × (Period / Max_Devices)
  - Example: 1 Hz, 5 devices → Device 1 at 0ms, Device 2 at 200ms, etc.
- **No acknowledgment required**

**Note:** For capability-specific status (logging, GPS, etc.), use dedicated capability CAN IDs (Base + 20+) instead.

### 4.2 Command Messages

Commands require acknowledgment for reliable delivery.

#### 4.2.1 Command Request (CAN ID Base + 2)

```
Byte 0:    Target Device ID (255 = broadcast to all)
Byte 1:    Command Code
Byte 2:    Transaction ID (for matching request/response)
Byte 3-7:  Command Parameters
```

**Standard Command Codes:**

**0x10: Read Parameter**

```
Byte 3:    Parameter ID (high byte)
Byte 4:    Parameter ID (low byte)
Byte 5-7:  Reserved
```

**0x11: Write Parameter**

```
Byte 3:    Parameter ID (high byte)
Byte 4:    Parameter ID (low byte)
Byte 5-7:  Parameter Value (up to 3 bytes, or use ISO-TP for larger)
```

**0x12: Execute Action**

```
Byte 3:    Action ID
Byte 4-7:  Action Parameters
```

**0x13: Reset Device**

```
Byte 3:    Reset Type (0x00=Soft, 0x01=Hard, 0x02=Factory)
Byte 4-7:  Reserved
```

**0x14: Status Request**

```
Byte 0:    Target Device ID (typically 255 for broadcast)
Byte 1:    0x14 (STATUS_REQUEST)
Byte 2:    Transaction ID
Byte 3-7:  Reserved
```

**Purpose:** Query device operational status. This is the only command that devices in VERSION_MISMATCH status will respond to.

Broadcast Behavior:

- When sent with Target ID = 255, all devices respond with staggered timing (5-10ms per device)
- Devices in VERSION_MISMATCH status respond even though they ignore all other protocol messages
- Devices in ONLINE status respond normally
- Responses use standard Command Response format (Base + 3)

**0x22: Get Config CRC32**

```
Byte 0:    Target Device ID
Byte 1:    0x22 (GET_CONFIG_CRC)
Byte 2:    Transaction ID
Byte 3:    Config Type (see Config Type Enumeration below)
Byte 4-7:  Reserved
```

Response contains CRC32 of requested configuration section.

**0x23: Get Config**

```
Byte 0:    Target Device ID
Byte 1:    0x23 (GET_CONFIG)
Byte 2:    Transaction ID
Byte 3:    Config Type (see Config Type Enumeration below)
Byte 4-7:  Reserved
```

Initiates ISO-TP transfer of configuration data from target device.

**Config Type Enumeration:**

```
0x00: Complete configuration (all sections)
0x01-0xFF: Application-specific config types (registered dynamically)
```

**0x30-0xFF: Application-Specific Commands**

Applications define command codes in this range based on their registered capabilities and operational requirements. Examples might include:
- Start/stop data streaming commands
- Calibration commands
- Mode change commands
- Capability-specific control commands

**Broadcast Command Handling:**

Most broadcast commands (Target ID = 255) SHALL NOT generate responses to avoid bus flooding.

**Exception:** Status Request (0x14) broadcast SHALL generate responses:

- All devices respond with staggered timing (5-10ms offset per Device ID)
- Stagger calculation: Response_Delay = (Device_ID - 1) × 10ms
- Example: Device 1 responds at 0ms, Device 2 at 10ms, Device 3 at 20ms, etc.
- This includes devices in VERSION_MISMATCH status

#### 4.2.2 Command Response (CAN ID Base + 3)

```
Byte 0:    Source Device ID
Byte 1:    Command Code (echo from request)
Byte 2:    Transaction ID (echo from request)
Byte 3:    Result Code (0x00=Success, 0x01=Error, 0x02=Invalid, etc.)
Byte 4-7:  Response Data or Error Details
```

**Result Codes:**

```
0x00: Success
0x01: Generic Error
0x02: Invalid Parameter
0x03: Unsupported Command
0x04: Timeout
0x05: CRC Error
0x06: Buffer Overflow
0x07: Device Busy
0x08: Access Denied
0x09: Not Ready
0x0A: Invalid State
0x10-0xFF: Application-specific errors
```

**Response Data Examples:**

**Read Parameter Response (0x10):**

```
Byte 3:    Result Code
Byte 4-7:  Parameter Value (up to 4 bytes)
```

**Status Request Response (0x14):**

```
Byte 0:    Source Device ID
Byte 1:    0x14 (STATUS_REQUEST)
Byte 2:    Transaction ID (echo from request)
Byte 3:    Current Status (see Device Status Enumeration below)
Byte 4:    Protocol Version (Major.Minor as uint8_t)
Byte 5:    Health Status (0x00=OK, 0x01=Warning, 0x02=Error)
Byte 6-7:  Reserved
```

Device Status Enumeration (Byte 3):

```
0x00: OFFLINE (not initialized)
0x01: INIT (initializing)
0x02: CLAIMING (claiming ID)
0x03: ONLINE (normal operation)
0x04: VERSION_MISMATCH (protocol version incompatible)
0x05: ERROR (fault detected)
```

**Get Config CRC32 Response (0x22):**

```
Byte 3:    Result Code
Byte 4-7:  CRC32 value (32-bit, LSB first)
```

**Get Config Response (0x23):**

```
Byte 3:    Result Code (0x00 = ISO-TP transfer will begin)
Byte 4-5:  Config data size (16-bit, bytes)
Byte 6-7:  Reserved
```

If successful, ISO-TP transfer of config data begins immediately on Base + 6.

#### 4.2.3 Retry Mechanism

- **Timeout**: 100ms for first attempt
- **Retry backoff**: 100ms, 200ms, 500ms
- **Max retries**: 3 attempts
- **On failure**: Log error, optionally notify application layer

### 4.3 State Change Notifications (CAN ID Base + 4)

**Purpose**: Event-driven state updates requiring confirmation

**Notification Message:**

```
Byte 0:    Source Device ID
Byte 1:    State Version Number (increments with each change)
Byte 2:    State Type/Category
Byte 3-7:  State Data
```

**State Type/Category Examples:**

```
0x01: Operating mode changed (IDLE/ACTIVE/SLEEP/ERROR)
0x02: Fault condition detected
0x03: Calibration status changed
0x04: Connection status changed
0x05: Threshold event (sensor crossed limit)
0x10-0xFF: Application-specific state changes
```

**Response (CAN ID Base + 5):**

```
Byte 0:    Source Device ID
Byte 1:    Target Device ID, device that is being acknowledged
Byte 2:    State Version Number (echo)
Byte 3:    Result Code (0x00=Success, 0x01=Error, 0x02=Invalid, etc.)
Byte 4-7:  Response Data or Error Details
```

**Behavior:**

- Sender retransmits if no ACK within 50ms
- Receivers discard duplicate state versions
- If device detects version gap, requests full state resync

-----

## 5. Bulk Data Transfer (ISO-TP Layer)

### 5.1 Use Cases

- Configuration files (max 4095 bytes per transfer)
- Log file retrieval
- Any data >7 bytes requiring segmentation

**Transfer Size Limit:** 4095 bytes maximum per ISO-TP transfer (imposed by 12-bit length field in First Frame). For larger data, split into multiple transfers or multiple Config Type sections.

### 5.2 ISO-TP Implementation (ISO 15765-2 Subset)

**CAN IDs:**

- Base + 6: Request (sender → receiver)
- Base + 7: Response (receiver → sender, for flow control and ACKs)

**Frame Types:**

#### Single Frame (SF) - Data ≤7 bytes
```
Byte 0:    0x0N (N = data length, 1-7)
Byte 1-7:  Data
```

#### First Frame (FF) - Start of multi-frame
```
Byte 0-1:  0x1FFF where FFF = total data length (12 bits, max 4095 bytes)
Byte 2-7:  First 6 bytes of data
```

#### Consecutive Frame (CF)
```
Byte 0:    0x2N (N = sequence number, 0-15, rolls over)
Byte 1-7:  Next 7 bytes of data
```

#### Flow Control (FC) - Receiver → Sender
```
Byte 0:    0x30 (Continue), 0x31 (Wait), or 0x32 (Abort)
Byte 1:    Block Size (0 = unlimited, 1-255 frames before next FC)
Byte 2:    Separation Time Min (STmin) in ms (0-127ms, or 0xF1-0xF9 for 100-900μs)
Byte 3-7:  Reserved
```

### 5.3 Transfer Protocol

**Sender Behavior:**

1. Send First Frame
1. Wait for Flow Control (timeout: 1000ms)
1. Send Consecutive Frames respecting Block Size and STmin
1. If Block Size > 0, wait for next Flow Control after each block
1. On Wait (0x31): Pause, wait for Continue Flow Control
1. On Abort (0x32) or timeout: Cancel transfer

**Receiver Behavior:**

1. Receive First Frame, allocate buffer
1. Send Flow Control (Continue with block size and timing)
1. Receive Consecutive Frames, check sequence numbers
1. Detect missing frames → send Abort Flow Control
1. After complete reception, validate data (CRC, structure)
1. Send final acknowledgment via Command Response (CAN ID Base + 3)

### 5.4 Bulk Transfer Framing

**Transfer Initiation (wraps ISO-TP)**

All ISO-TP data payloads start with:
```
Byte 0:    Source Device ID
Byte 1:    Target Device ID
Byte 2:    Transfer Type (0x01=Config Write, 0x02=Config Read, etc.)
Byte 3:    Transaction ID
Byte 4-N:  Actual data (binary config file, etc.)
```

**Transfer Completion ACK (CAN ID Base + 3):**
```
Byte 0:    Target Device ID
Byte 1:    0xFE (bulk transfer ACK, not command response)
Byte 2:    Transaction ID
Byte 3:    Result (0x00=Success, 0x01=CRC Error, 0x02=Invalid Data, etc.)
Byte 4-7:  Reserved
```

### 5.5 Timing and Performance

**Targets** (at 1 Mbps):

- 100-byte transfer: ~0.5-1 seconds
- 1000-byte transfer: ~2-5 seconds
- Max single transfer: 4095 bytes (ISO-TP limit)

**Error Handling:**

- CRC-16 or CRC-32 appended to data payload
- Validation before committing configuration
- Atomic updates (write to temp buffer, validate, then apply)

---

## 6. Protocol Status Machine

### 6.1 Device Statuses

```
OFFLINE          → Power off or not initialized
INIT             → Power on, generating ID and unique identifier (if needed)
CLAIMING         → Attempting to claim Device ID
ONLINE           → Normal operation, ID claimed
VERSION_MISMATCH → Incompatible protocol version detected (network-wide condition)
ERROR            → Fault detected, limited functionality
```

**VERSION_MISMATCH Status Behavior:**

- All protocol transmissions disabled:
  - No heartbeat messages
  - No status broadcasts
  - No command requests
  - No state change notifications
  - No capability data streams
- Exception: Device MUST respond to Status Request command (0x14) only
- DBC-defined messages (0x100-0x6FF) remain fully operational
- Recovery: Requires device reboot
- Application layer should indicate error condition (LED, display, etc.)

### 6.2 Status Transitions

```
OFFLINE → INIT:                Power on / reset
INIT → CLAIMING:               After startup backoff and Discovery
CLAIMING → ONLINE:             ID claim successful (no conflicts, compatible version)
CLAIMING → VERSION_MISMATCH:   Received 0x02 (Version Incompatible) response to ID_CLAIM
CLAIMING → INIT:               ID conflict, retry
ONLINE → VERSION_MISMATCH:     Detected incompatible version in ID_CLAIM message from new device
ONLINE → ERROR:                Critical fault detected
ERROR → INIT:                  Fault cleared, reinitialization
ONLINE → OFFLINE:              Shutdown command or power loss
VERSION_MISMATCH → OFFLINE:    Device reboot required (no automatic recovery)
```

-----

## 7. Error Handling and Reliability

### 7.1 Message Loss Detection

- **Sequence numbers**: Status messages include rolling counter
- **Transaction IDs**: Command/response matching
- **State versions**: Detect missed state changes
- **Timeouts**: All request/response pairs have defined timeouts

### 7.2 Retry Strategy

|Message Type       |Timeout|Retries|Backoff               |
|-------------------|-------|-------|----------------------|
|Discovery Request  |200ms  |3      |100ms, 200ms, 500ms   |
|Command Request    |100ms  |3      |100ms, 200ms, 500ms   |
|State Change       |50ms   |3      |50ms, 100ms, 200ms    |
|ISO-TP Flow Control|1000ms |1      |N/A (abort on timeout)|
|Heartbeat          |N/A    |N/A    |Not retransmitted     |

### 7.3 Error Codes

```
0x00: Success
0x01: Generic Error
0x02: Invalid Parameter
0x03: Unsupported Command
0x04: Timeout
0x05: CRC Error
0x06: Buffer Overflow
0x07: Device Busy
0x08: Access Denied
0x10-0xFF: Application-specific errors
```

-----

## 8. Configuration Management

### 8.1 Configuration Storage

- Devices maintain persistent configuration (EEPROM, Flash)
- Configuration transmitted as binary data via ISO-TP
- Each configuration has version number and CRC

### 8.2 Configuration Read

**Using Get Config Command (0x23):**

```
1. Requester sends Command Request (Base + 2):
   [Target_ID][0x23][Transaction_ID][Config_Type][0x00][0x00][0x00]

2. Target responds (Base + 3):
   [Source_ID][0x23][Transaction_ID][0x00][Size_Low][Size_High][0x00][0x00]
   
3. Target initiates ISO-TP transfer on Base + 6:
   Sends configuration data via ISO-TP
   
4. Requester validates received data (CRC check)

5. Requester sends Transfer ACK (Base + 3):
   [Target_ID][0xFE][Transaction_ID][Result][0x00][0x00][0x00][0x00]
```

**Using Get Config CRC Command (0x22) - For Verification:**

```
1. Requester sends Command Request (Base + 2):
   [Target_ID][0x22][Transaction_ID][Config_Type][0x00][0x00][0x00]

2. Target responds (Base + 3):
   [Source_ID][0x22][Transaction_ID][0x00][CRC32 bytes 0-3]
   
3. Requester compares CRC with local copy to detect changes
```

### 8.3 Configuration Write

```
1. Requester initiates ISO-TP transfer on Base + 6 with config data:
   First frame contains: [Source_ID][Target_ID][0x01][Transaction_ID][Data...]
   (Transfer Type 0x01 = Config Write)
   
2. Target receives complete data via ISO-TP

3. Target validates:
   - Data structure is valid
   - CRC32 matches (appended to config data)
   - Config type and parameters are acceptable
   
4. Target writes to temporary buffer

5. Target performs full validation of config semantics

6. Target commits to persistent storage (EEPROM/Flash)

7. Target sends Transfer ACK via Base + 3:
   [Source_ID][0xFE][Transaction_ID][Result][0x00][0x00][0x00][0x00]
   
8. If config requires restart, target may:
   - Send State Change Notification (Base + 4) about impending reboot
   - Reboot after brief delay
```

### 8.4 Configuration Sections

To minimize transfer size, configuration may be split into logical sections identified by registered Config Type identifiers.

**Dynamic Config Type System:**

Applications register config types during device initialization:
```
register_config_type(type_id, description, handler)

Convention:
- 0x00: Complete configuration (all sections combined)
- 0x01-0xFF: Application-defined config sections
```

**Registration Requirements:**

- Config types must be registered during device initialization before normal operation begins
- All devices on the network must follow the same application-specific Config Type enumeration
- This ensures consistent interpretation of Config Type values across the network

Each registered config type has associated handlers for:
- Serialization (convert config to binary format)
- Deserialization (parse binary format to config structure)
- Validation (verify config data integrity and semantics)
- Storage (read/write to persistent memory)

Each section is independently transferable by specifying Config Type in Get Config (0x23) command.

**Config Type 0x00 Behavior:**

When Config Type 0x00 (Complete configuration) is requested, the device returns all configuration sections concatenated with section headers:
```
[Section_1_Type][Section_1_Length_Low][Section_1_Length_High][Section_1_Data...]
[Section_2_Type][Section_2_Length_Low][Section_2_Length_High][Section_2_Data...]
...
```

Length fields are 16-bit values indicating section data size in bytes (excluding header).

---

## 9. Implementation Requirements

### 9.1 Mandatory Features (All Protocol-Aware Devices)

- ID assignment protocol (discovery, claim, conflict resolution)
- Heartbeat transmission
- Status broadcast (if applicable)
- Command request/response handling
- Basic error handling and retries
- Capability registration system (if device has capabilities)
- Config type registration system (if device has configurable parameters)

### 9.2 Optional Features

- ISO-TP bulk transfer (required only if config >7 bytes)
- State change notifications (if device has state to share)
- Discovery response (helpful but not critical)
- Advanced error recovery
- Capability implementations (application-specific)

### 9.3 Resource Requirements (Estimated)

**RAM:**

- Minimal: 256 bytes (buffers, status machine)
- With ISO-TP: 1-4 KB (transfer buffers)

**Flash:** 5-15 KB (protocol stack code)

**CPU:** <5% at 1 Hz status updates, <20% during ISO-TP transfer

### 9.4 Backward Compatibility

- Simple devices ignore Base + 0 through Base + 99 completely
- Protocol-aware devices process both normal CAN (0x100-0x6FF) and management protocol
- Capability streams (Base + 20+) are ignored by devices without corresponding capability
- No impact on existing DBC-defined message timing or priority
- Base CAN ID is configurable to avoid conflicts with existing systems

-----

## 10. Testing and Validation

### 10.1 Test Cases

1. **ID Assignment**: 5 devices power up simultaneously, verify unique IDs
1. **ID Conflict**: Two devices claim same ID, verify proper resolution
1. **Random ID Generation**: Device without unique ID generates valid random identifiers
1. **Fast Startup**: Measure startup time from power-on to first heartbeat
1. **Heartbeat Loss**: Disconnect device, verify timeout detection
1. **Command Retry**: Simulate message loss, verify 3 retries with backoff
1. **ISO-TP Transfer**: Send 1KB config file, verify integrity
1. **ISO-TP Flow Control**: Test receiver flow control under load
1. **State Synchronization**: Verify state version gap detection and recovery
1. **Bus Coexistence**: Run protocol alongside heavy sensor traffic (80% bus load)
1. **Capability Discovery**: Verify capability flags in heartbeat
1. **Multi-device Capability**: Test time-slotting with 3 devices sharing same capability

**Version Incompatibility Handling:**

**Scenario 1:** Network with 3 devices (v1.0), new device (v2.0) attempts to join

- Verify all v1.0 devices enter VERSION_MISMATCH status
- Verify new device receives 0x02 response and enters VERSION_MISMATCH
- Verify Status Request (0x14) broadcast receives responses from all devices
- Verify all devices report VERSION_MISMATCH status
- Verify DBC messages continue operating normally
- Verify no heartbeats, status broadcasts, or other protocol messages transmitted


**Scenario 2:** Empty network, first device boots with any version

- Verify device successfully claims ID and goes ONLINE
- Second device with same version joins
- Verify normal operation


**Scenario 3:** Status Request command

- Broadcast Status Request to VERSION_MISMATCH network
- Verify all devices respond with staggered timing
- Verify responses contain correct state (0x04) and version information

### 10.2 Performance Metrics

- ID assignment time: <300ms (typical with responses), <1.2 seconds (empty network, 3 retries)
- Command round-trip latency: <50ms (typical), <200ms (with retries)
- 1KB config transfer: ~2-5 seconds at 1 Mbps
- Bus utilization: Management protocol <5% at 1 Hz status updates
- Capability streams: Variable based on implementation (example: 50ms update rate: ~1-2% per device)

-----

## 11. Protocol Versioning

### 11.1 Version Field

- **Major.Minor** format (uint8_t: upper 4 bits = major, lower 4 bits = minor)
- Current version: 1.0 (0x10)
- Included in ID_CLAIM message

### 11.2 Compatibility Rules

- **Major version change**: Breaking changes, incompatible
- **Minor version change**: Backward compatible additions

**Version Compatibility Enforcement:**
- Devices with different major protocol versions are incompatible
- When incompatible version detected, **entire network** enters VERSION_MISMATCH status
- Detection occurs during ID_CLAIM phase:
  - New device sends ID_CLAIM with its protocol version
  - Existing ONLINE devices compare major version
  - If major version differs:
    * Existing devices enter VERSION_MISMATCH status
    * One device sends ID_CLAIM_RESPONSE with result 0x02
    * New device enters VERSION_MISMATCH status upon receiving 0x02
    * All devices require reboot to recover
- Minor version differences within same major version are tolerated:
  - Higher minor version devices should provide backward compatibility
  - Lower minor version devices may lack new features but remain functional
  
**Rationale**: Network-wide VERSION_MISMATCH prevents partial operation with mixed protocol versions, which could cause unpredictable behavior or data corruption.

**Recovery**: Upgrade/downgrade all devices to compatible protocol version, then reboot network.

-----

## 12. Security Considerations

### 12.1 Current Scope (No Security)

- No authentication or encryption
- Any device can claim any ID
- Any device can send commands to any other
- Suitable for trusted, physically secure networks only

### 12.2 Future Extensions (Out of Scope)

- Message authentication codes (MAC)
- Challenge-response authentication
- Encrypted configuration data
- Role-based access control

-----

## 13. Documentation and Tooling

### 13.1 Required Documentation

- Protocol specification (this document)
- Implementation guide with code examples
- DBC file defining management protocol CAN IDs and formats
- Configuration file format specification and registration API

### 13.2 Recommended Tooling

- CAN bus analyzer (CANalyzer, Busmaster) with protocol decoder
- Configuration file encoder/decoder utilities
- Automated test suite for conformance testing
- Configuration management GUI tool

-----

## 14. Capability-Based Data Streaming

### 14.1 Overview

Devices can implement application-defined capabilities that are automatically discovered via heartbeat messages. Each capability reserves a dedicated CAN ID and defines its own data format and update rate. The protocol provides only the framework for capability registration, discovery, and time-slotted transmission—all specific capability definitions are application-specific.

### 14.2 Capability Discovery

**Heartbeat Capability Flags (Bytes 4-7 of Heartbeat message):**

```
Bits 0-31: Application-defined capability flags (32 bits, LSB first)
```

Each bit represents a registered capability. The mapping between bit positions, capability identifiers, and CAN IDs is defined by the application during capability registration. When a device sets a capability bit to 1, it indicates:

1. The device implements this capability
1. The device will transmit data on the corresponding registered CAN ID
1. Other devices can expect periodic updates on that CAN ID

### 14.3 Dynamic Capability Registration

Applications register capabilities during device initialization:

```
register_capability(capability_bit_number, data_format, update_rate)

Where:
- capability_bit_number: Bit position in heartbeat flags (0-31)
- data_format: Application-defined structure
- update_rate: Transmission period in milliseconds

Returns: CAN ID in form Base + 20 + capability_bit_number
```

**Registration Rules:**

- CAN ID is automatically derived: CAN_ID = Base + 20 + capability_bit_number
- Each capability_bit (0-31) maps to exactly one CAN ID (Base + 20 through Base + 51)
- Multiple devices can register the same capability (same bit number = same CAN ID)
- Registration occurs during device initialization and follows network-wide conventions

**Example Registration Sequence:**
```c
// Device firmware initialization
register_capability(0, format_logging, 1000);      // Bit 0 → Base+20, 1Hz
register_capability(1, format_gps, 100);           // Bit 1 → Base+21, 10Hz
register_capability(10, format_custom_sensor, 50); // Bit 10 → Base+30, 20Hz
```

---

### 14.4 Time-Slotting for Shared Capability CAN IDs

When multiple devices share a capability CAN ID, they use time-slotting based on Device ID to prevent collisions.

**Time-Slot Calculation:**
```
Slot_Index = (Device_ID - 1) mod Max_Devices_Per_Capability
Time_Offset = Slot_Index × (Update_Period / Max_Devices_Per_Capability)

Where:
- Device_ID: The device's assigned ID (1-254)
- Max_Devices_Per_Capability: Expected maximum devices sharing this capability (typically 4-8)
- Update_Period: Capability update rate in milliseconds
```

**Example:** Three devices (IDs 1, 3, 5) share a capability with 100ms update period, Max_Devices_Per_Capability = 8:
```
Device 1: Slot = (1-1) mod 8 = 0, Offset = 0 × (100ms/8) = 0ms
Device 3: Slot = (3-1) mod 8 = 2, Offset = 2 × (100ms/8) = 25ms
Device 5: Slot = (5-1) mod 8 = 4, Offset = 4 × (100ms/8) = 50ms

Timeline:
0ms:    Device 1 transmits capability data
25ms:   Device 3 transmits capability data
50ms:   Device 5 transmits capability data
100ms:  Next period starts (Device 1 transmits again)
125ms:  Device 3 transmits
150ms:  Device 5 transmits
...
```

**Implementation Notes:**

- Max_Devices_Per_Capability is configured per capability during registration
- Typical values: 4 for high-rate capabilities (≤100ms), 8 for medium-rate (≤1000ms)
- Devices should add small random jitter (±5ms) to reduce systematic collisions
- If actual device count exceeds Max_Devices_Per_Capability, slots wrap but collisions may occur
- CAN arbitration handles any remaining collisions based on Device ID priority

---

**Simplified Algorithm (Conservative):**

For simpler implementation, devices can use:
```
Time_Offset = (Device_ID % Max_Expected_Devices) × (Update_Period / Max_Expected_Devices)
This ensures spacing even when exact Active_Device_Count is unknown, at the cost of potentially underutilizing the available bandwidth.
```

14.5 Example Capability Implementations

The protocol does not mandate specific capabilities. Here are examples of how applications might define capabilities:

**Example:** Logging Capability

**Registration:**
```c
#define CAPABILITY_LOGGING 0  // Bit 0
#define LOGGING_CAN_OFFSET 20 // Base + 20
register_capability(CAPABILITY_LOGGING, LOGGING_CAN_OFFSET, logging_handler, 1000);
```

**Message Format (Application-Defined):**
```
Byte 0:    Device ID
Byte 1:    Logging Status (0x00=Ready, 0x01=Active, 0x02=Paused, 0x03=Full, 0x04=Error)
Byte 2:    Health Status (0x00=OK, 0x01=Warning, 0x02=Error)
Byte 3-7:  Application-specific logging data
Example: GPS Capability
Registration:
c#define CAPABILITY_GPS 1     // Bit 1
#define GPS_CAN_OFFSET 21    // Base + 21
register_capability(CAPABILITY_GPS, GPS_CAN_OFFSET, gps_handler, 100);
```

**Message Format (Application-Defined):**
```
Byte 0:    Device ID
Byte 1:    Fix Status (0x00=No fix, 0x01=2D fix, 0x02=3D fix)
Byte 2-4:  Latitude (24-bit signed, scaled)
Byte 5-7:  Longitude (24-bit signed, scaled)
```

Applications are free to define any capability structure that fits their needs.

---

### 14.6 Capability Command Integration

Applications can define capability-specific commands using the standard command framework (0x30-0xFF command codes). The protocol provides the transport mechanism; applications define command semantics for each registered capability.

---

## Appendix A: Message Type Summary

### Management Messages (Base + 0)

| Message Type | Code | Description |
|--------------|------|-------------|
| ID_CLAIM | 0x01 | Device claims Device ID |
| ID_CLAIM_RESPONSE | 0x02 | Response to ID claim (conflict) |
| DISCOVERY_REQUEST | 0x03 | Request network enumeration |
| DISCOVERY_RESPONSE | 0x04 | Response with device info |
| HEARTBEAT | 0x05 | Periodic liveness indication |

### ID Claim Response Result Codes (Base + 0)

| Code | Description |
|------|-------------|
| 0x00 |  Accept (no conflict) |
| 0x01 | Reject/Conflict (ID already in use) |
| 0x02 | Version Incompatible |

### Command Codes (Base + 2 Request, Base + 3 Response)

| Command Code | Description |
|--------------|-------------|
| 0x10 | Read Parameter |
| 0x11 | Write Parameter |
| 0x12 | Execute Action |
| 0x13 | Reset Device |
| 0x14 | Status Request |
| 0x22 | Get Config CRC32 |
| 0x23 | Get Config |
| 0x30-0xFF | Application-Specific Commands |

### Special Response Codes

| Code | Description |
|------|-------------|
| 0xFE | Bulk Transfer ACK (ISO-TP completion) |

### Device Status Enumeration (for Status Request response)

| Code | Status | Description |
|------|--------|-------------|
| 0x00 | OFFLINE | Not initialized |
| 0x01 | INIT | Initializing |
| 0x02 | CLAIMING | Claiming ID |
| 0x03 | ONLINE | Normal operation |
| 0x04 | VERSION_MISMATCH | Protocol version incompatible |
| 0x05 | ERROR | Fault detected |

---

## Appendix B: Example Message Flows

### B.1 Device Startup and ID Assignment
```
New Device:
  → Discovery Request (Base + 0): [0x03][0x00]...[0x00]
  
Existing Devices (staggered 5-10ms):
  ← Discovery Response (Base + 0): [0x04][ID_1][UID_1]...[Type_1][0x00]
  ← Discovery Response (Base + 0): [0x04][ID_2][UID_2]...[Type_2][0x00]
  ← Discovery Response (Base + 0): [0x04][ID_5][UID_5]...[Type_5][0x00]

New Device (selects ID=3, fills gap):
  → ID Claim (Base + 0): [0x01][0x03][UID_New]...[Type][Ver]
  
  Wait 50ms for objections...
  
New Device:
  → Heartbeat (Base + 0): [0x05][0x03][0x00][Uptime][Caps...]
  Begin normal operation
```

### B.2 Command Request/Response
```
Controller:
  → Command Request (Base + 2): [Target_ID][0x11][TxID][Param_H][Param_L][Val...]

Target Device:
  ← Command Response (Base + 3): [Source_ID][0x11][TxID][0x00][Reserved...]
  (0x00 = Success)
```

### B.3 Configuration Transfer
```
Controller:
  → Command Request (Base + 2): [Target][0x23][TxID][ConfigType][0x00][0x00][0x00]

Target:
  ← Command Response (Base + 3): [Source][0x23][TxID][0x00][Size_L][Size_H][0x00][0x00]

Target → Controller (ISO-TP on Base + 6):
  First Frame: [0x1FFF][Source][Target][0x02][TxID][Config_Data...]
                      ↑ Transfer Type 0x02 = Config Read response
  
Controller:
  ← Flow Control (Base + 7): [0x30][BlockSize][STmin][0x00]...

Target continues sending Consecutive Frames...

Controller validates received data (CRC check), then:
  → Transfer ACK (Base + 3): [Target][0xFE][TxID][0x00][0x00][0x00][0x00][0x00]
```

### B.4 Capability Data Streaming
```
Device 1 (with registered capability, bit 0 set):
  → Capability Data (Base + 20): [Device_ID][Status][Data...]
  Transmits at registered update rate with time-slot offset
  
Device 3 (same capability):
  → Capability Data (Base + 20): [Device_ID][Status][Data...]
  Transmits with different time-slot offset to avoid collision
```

### B.5 Version Mismatch Detection

```
Network State: Devices 1, 2, 3 running protocol v1.0, all ONLINE

New Device (v2.0) joins:
  → Discovery Request (Base + 0): [0x03][0x00]...[0x00]
  
Existing Devices respond:
  ← Discovery Response: [0x04][0x01][UID_1]...[0x10] (Device 1, v1.0)
  ← Discovery Response: [0x04][0x02][UID_2]...[0x10] (Device 2, v1.0)  
  ← Discovery Response: [0x04][0x03][UID_3]...[0x10] (Device 3, v1.0)

New Device (v2.0):
  → ID Claim (Base + 0): [0x01][0x04][UID_New]...[Type][0x20] (v2.0)

Device 1 (lowest ID):
  Detects version mismatch (0x20 vs 0x10)
  Enters VERSION_MISMATCH status
  ← ID_CLAIM_RESPONSE: [0x02][0x01][0x04][0x02][0x00]... (Version Incompatible)

Devices 2 and 3:
  Detect version mismatch
  Enter VERSION_MISMATCH status
  Do NOT send response

New Device (v2.0):
  Receives 0x02 response
  Enters VERSION_MISMATCH status

Later, diagnostic tool queries network:
  → Status Request (Base + 2): [0xFF][0x14][0x42][0x00]...[0x00]

All devices respond:
  ← t=0ms:   [0x01][0x14][0x42][0x04][0x10][0x00]... (Device 1: VERSION_MISMATCH, v1.0)
  ← t=10ms:  [0x02][0x14][0x42][0x04][0x10][0x00]... (Device 2: VERSION_MISMATCH, v1.0)
  ← t=20ms:  [0x03][0x14][0x42][0x04][0x10][0x00]... (Device 3: VERSION_MISMATCH, v1.0)
  ← t=30ms:  [0x04][0x14][0x42][0x04][0x20][0x00]... (Device 4: VERSION_MISMATCH, v2.0)
```

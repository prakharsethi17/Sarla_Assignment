# Student Record Management System

## Overview
This project implements a distributed **Student Record Management System** using C++17 and the **Central Broker** architectural pattern. The system is designed to demonstrate high-performance asynchronous networking, task scheduling, and decoupled data processing. It consists of a Central Server (Broker), a Producer Node (Worker), and a Consumer Node (Client), all communicating via **WebSockets**.

## Architecture

The system follows a star topology where the **Central Server** acts as the intermediary for all communications.

**Topology:**
`Consumer` <---> `Central Server` <---> `Producer`

### Component Roles

| Component | Role | Responsibilities |
| :--- | :--- | :--- |
| **Central Server** | Broker & Scheduler | 1. Manages a FIFO Job Queue.<br>2. Tracks Producer availability (Idle/Busy).<br>3. Dispatches jobs sequentially to the Producer.<br>4. Maintains a history registry of processed results. |
| **Producer Node** | Worker | 1. Connects to the Server as a client.<br>2. Caches the CSV dataset in memory for sub-millisecond access.<br>3. Executes operations (List, Search, Sort).<br>4. Uploads processed data and performance metrics to the Server. |
| **Consumer Node** | Controller & Viewer | 1. Submits processing jobs to the Server.<br>2. Lists historical jobs sorted by timestamp.<br>3. Downloads and visualizes results (Tables & Metrics). |

## Dependencies

The project relies on industry-standard C++ libraries managed via **vcpkg**.

| Library | Version | Purpose |
| :--- | :--- | :--- |
| **Boost.Beast** | 1.80+ | WebSocket protocol implementation (Ref: handling handshakes and framing). |
| **Boost.Asio** | 1.80+ | Asynchronous I/O networking context (`io_context`). |
| **nlohmann/json** | 3.11+ | JSON serialization and deserialization for network messages. |
| **GoogleTest** | 1.14+ | Unit testing framework for core logic verification. |
| **fast-cpp-csv-parser** | Latest | High-performance CSV parsing library. |

## Process Workflow

### 1. System Startup
1.  **Server** starts on Port 9002 (Configurable) and initializes the Job Queue.
2.  **Producer** connects to the Server, sends a `register_producer` command, and enters an **Idle** state, awaiting instructions.
3.  **Consumer** connects to the Server to issue commands.

### 2. Job Execution Cycle
1.  **Submission**: The Consumer submits a job (e.g., `{"operation": "sort", "field": "age"}`) to the Server.
2.  **Queuing**: The Server assigns a Job ID and pushes the request to the internal queue.
3.  **Dispatch**: 
    *   If the Producer is **Idle**, the Server immediately sends a `dispatch_job` message.
    *   If the Producer is **Busy**, the job remains in the queue.
4.  **Processing**: The Producer processes the cached data (filtering/sorting) and measures performance (Parse Time, Process Time, Memory Usage).
5.  **Upload**: The Producer sends an `upload` message containing the result data and metrics back to the Server.
6.  **Archival**: The Server saves the result to a file (e.g., `data/history/<timestamp>.csv`) and records the metrics in the registry.
7.  **Completion**: The Server marks the Producer as **Idle** and checks the queue for the next job.

## Architectural Decisions & Trade-offs

| Decision | Alternative | Rationale | Trade-off |
| :--- | :--- | :--- | :--- |
| **Central Broker** | Peer-to-Peer (P2P) | Decouples Consumers from Producers. Allows the broker to manage load balancing and job avenues (e.g., re-dispatching if a worker fails). | **Single Point of Failure (SPOF)**: If the Server fails, the entire system halts. |
| **WebSockets** | Raw TCP / gRPC | Provides a persistent, full-duplex connection suitable for real-time signaling. Easier to debug than raw TCP. | Higher protocol overhead compared to raw TCP sockets due to framing and masking. |
| **JSON Messaging** | Protocol Buffers | Human-readable and easy to debug without compilation steps. Sufficient for the required dataset size. | **Serialization Cost**: JSON parsing is significantly slower and more verbose than binary formats (Protobuf/FlatBuffers). |
| **Asynchronous I/O** | Threads | Boost.Asio allows the Server to handle thousands of concurrent connections on a single thread. | **Complexity**: Asynchronous control flow (callbacks/coroutines) is harder to reason about than blocking code. |
| **In-Memory Caching** | Disk Reads | The Producer loads the CSV once. Subsequent queries are instant. | **Memory Usage**: The entire dataset must fit in RAM. Not suitable for multi-gigabyte files without paging. |

## Configuration

The system is configured via `bin/config.json`:

```json
{
    "server": { "port": 9002 },
    "producer": { "host": "127.0.0.1", "port": 9002 },
    "consumer": { "host": "127.0.0.1", "port": 9002 }
}
```

## Build Instructions

Using **CMake** and **PowerShell**:

1.  **Build**:
    ```powershell
    .\build.ps1
    ```
    This script will invoke CMake, use the vcpkg toolchain, compile all targets, and move executables to `bin/`.

2.  **Generate Data**:
    ```powershell
    .\bin\generate_students.exe
    ```

3.  **Run Tests**:
    ```powershell
    .\bin\unit_tests.exe
    ```

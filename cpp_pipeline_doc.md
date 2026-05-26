# Engineering Specification: Ultra-Low Latency C++20 POSIX Data Pipeline

## Objective
Implement a high-performance, two-process quantitative execution framework using native POSIX IPC channels and C++20 design paradigms. The system avoids graphical and network stack overhead by utilizing memory-backed Named Pipes (`mkfifo`) to simulate low-latency multi-instrument data ingestion and triage routing.

---

## Technical Architecture & Constraints

* **Language Standard:** Strict C++20. Use modern concepts, structured bindings, and `<chrono>` types.
* **Operating System:** POSIX-compliant (Target: macOS/Darwin, ARM64 architecture).
* **Concurrency Model:** Lock-free design where possible, maximizing cache locality and utilizing native OS system calls directly over third-party wrapper libraries.
* **Data Transport:** High-throughput POSIX Named Pipe (`mkfifo`) acting as a unidirectional, byte-stream memory buffer between separate binary executables.

---

## Parameter Configuration

Configure the global parameters within an accessible namespace or config header:
* `TICKER_SET`: `{"SOL", "ARB", "LINK"}`
* `INGESTION_INTERVAL_MS`: `10` (Simulates a fast 100Hz high-frequency WebSocket stream)
* `VOL_DELTA_MIN` / `MAX`: `-500` to `500` (Discrete values representing aggressive market order imbalances)
* `OBI_MIN` / `MAX`: `0` to `100` (Order Book Imbalance percentage)
* `SIGNAL_THRESHOLD_VOL_DELTA`: `> 250`
* `SIGNAL_THRESHOLD_OBI`: `> 80`
* `PIPE_PATH`: `"/tmp/trading_pipeline.fifo"`

---

## Component Specifications

### 1. Process 1: The High-Frequency Data Engine (`engine`)
This binary acts as the ingestion simulator. It continuously generates randomized trade and order book metrics for the designated asset universe and streams them sequentially down the pipe.

* **Initialization:**
    * Check if `PIPE_PATH` exists. If not, instantiate it via the POSIX `mkfifo(path, 0666)` system call.
    * Open the FIFO file descriptor in write-only mode (`std::ofstream` or native Unix `open()` with `O_WRONLY`). Note: This will naturally block until Process 2 opens the read end.
* **Execution Loop:**
    * Run an infinite loop throttled precisely to `INGESTION_INTERVAL_MS` using `std::this_thread::sleep_for`.
    * Iterate through the `TICKER_SET`. For each ticker, generate pseudo-random integers representing the current state using standard `<random>` engines (e.g., `std::mt19937`).
    * **Data Serialization:** Serialize the metrics into a dense, newline-terminated string payload. Use a clean, easily-parsed structure (e.g., `TICKER,VolDelta,OBI`).
    * **Pipe Transmission:** Write the string directly to the FIFO buffer. Call `flush()` immediately to guarantee zero buffer caching and force immediate kernel delivery to the reader.

### 2. Process 2: The Triage Scanner (`triage`)
This binary acts as the real-time evaluation and hotkey-ready execution trigger gate. It handles input parsing, metric verification, and optimized terminal rendering.

* **Initialization:**
    * Open the existing `PIPE_PATH` in read-only mode (`std::ifstream` or `open()` with `O_RDONLY`).
* **Execution Loop:**
    * Perform blocking line-by-line reads from the FIFO stream (e.g., using `std::getline`). This ensures zero CPU usage during microsecond gaps between ticks.
    * **Parsing Pipeline:** Parse the raw incoming string tokens using modern C++20 parsing idioms (such as `std::string_view` or `std::sscanf` for speed, avoiding costly `std::regex` heap allocations). Extract ticker name, Volume Delta, and OBI.
    * **Risk & Threshold Evaluation:** Compare the extracted variables against `SIGNAL_THRESHOLD_VOL_DELTA` and `SIGNAL_THRESHOLD_OBI` simultaneously.
    * **Anharmonic Terminal Rendering:** * If the criteria match, output a bright, high-visibility alert to `std::cout` using ANSI escape codes: Bold Green `\033[1;32m[EXECUTE LONG]\033[0m` followed by the structured metrics.
        * If the criteria do not match, print the running telemetry line in a muted gray `\033[90m[Monitoring]\033[0m` to keep the terminal clutter-free while maintaining a live pulse.

---

## Prompt Instructions for the IDE Agent

1.  **Project Layout:** Generate a clean directory structure containing a shared `Config.hpp` header, `Engine.cpp`, `Triage.cpp`, and a minimal, optimized `CMakeLists.txt` file configured with `-O3` optimization flags.
2.  **Robust Error Handling:** Ensure proper management of broken pipes (`SIGPIPE`). If the reader disconnects, the engine should handle the signal gracefully instead of crashing.
3.  **Modern Idioms:** Prioritize use of `std::string_view` for zero-copy string manipulation during parsing in the triage component to minimize allocation latency.
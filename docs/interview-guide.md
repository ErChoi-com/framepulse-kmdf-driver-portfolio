# Interview Guide

## How to position the project

Describe FramePulse as a deliberately scoped kernel-mode systems project designed to prove driver fundamentals end to end:

- buildable KMDF code
- a user-kernel control path
- installation and test-signing workflow
- WinDbg bring-up and request-path debugging
- synchronization and telemetry choices you can defend technically

That framing is stronger than overselling it as a fake GPU driver.

## Mapping to the AMD posting

### Kernel C/C++

You wrote a KMDF driver in C++ with explicit control over request handling and shared state.

### Debugging with WinDbg and tools

You can show how to set breakpoints on driver entry points, inspect WDF objects, examine the active thread and call stack, and verify symbol loading.

### OS internals

You can discuss:

- user/kernel transitions through `DeviceIoControl`
- synchronization via `WDFWAITLOCK`
- queue dispatch semantics
- process and thread IDs captured in kernel context
- timestamping and bounded logging in kernel memory

### Interfacing with user-mode software

The SetupAPI-based client discovers the published device interface and exercises a versioned IOCTL contract from user mode.

### Stability and security mindset

The interface is versioned, buffers are size-checked, and telemetry storage is bounded instead of untrusted or dynamically growing.

## Strong talking points

- Why you chose sequential dispatch first, and what would change for higher concurrency
- Why the shared header is versioned and why that matters in a large driver stack
- Why a ring buffer is useful for post-failure forensics without unbounded memory growth
- How you would evolve this into ETW + GPUView correlation in a graphics environment
- How you would split control, telemetry, and performance-critical paths in a larger production driver

## Best resume bullets

- Built a KMDF Windows kernel driver in C++ with a versioned IOCTL interface, bounded telemetry ring buffer, and SetupAPI-based user-mode validation tool.
- Automated test-signing, INF-based installation, and VM bring-up workflows for repeatable kernel debugging with WinDbg.
- Implemented synchronized driver state management and request accounting to demonstrate kernel debugging, OS internals, and user/kernel interface design.

## If they ask “what would you build next?”

Good answers:

- asynchronous request completion and cancellation support
- ETW events for GPUView correlation
- a multi-threaded stress harness with latency histograms
- a simulated DMA submission path with fence-style completion bookkeeping

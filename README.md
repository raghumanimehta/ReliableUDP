# Reliable UDP

## Goal
- Implement a reliable file transfer system over **UDP** using custom logic to handle:
  - Packet loss
  - Reordering
  - Duplicate suppression

## Stretch Goal
- Develop a simple **web-based** or **desktop UI** to improve usability and allow file transfers via a graphical interface.

---

## TODO

### Core Functionality
- [x] Set up basic UDP client and server
- [x] Implement file reading and sending logic on the client
- [x] Implement file writing logic on the server
- [x] Add packet sequence numbering
- [x] Implement basic ACK system
- [x] Add retransmission mechanism with timeouts
- [x] Define and document end-of-file signaling

### Testing and Validation
- [x] Test file transfers under normal conditions

### 🧰 Stretch Features
- [x] Basic CLI interface to choose file and mode

### Handshake protocol 
Compact alternating 3‑way handshake (packet, seq, and state changes):

- **Sender (IDLE → SYN_SENT):** sends `SYN` (seq=0); waits for `SYN-ACK` (retries on timeout).
- **Receiver (IDLE → SYN_RECV → ACK_SENT):** receives `SYN` (seq=0), verifies seq; sends `SYN-ACK` (seq=1); waits for `ACK`.
- **Sender (SYN_SENT → SYN_ACK → ACK_SENT → CONNECTED):** receives `SYN-ACK` (seq=1), verifies seq; sends `ACK` (seq=1) and transitions to `CONNECTED`.
- **Receiver (ACK_SENT → CONNECTED):** receives `ACK` (seq=1), verifies seq; transitions to `CONNECTED` (ready to transfer).

Notes: retries/timeouts and seq verification are applied at each wait step; mismatches trigger retries or aborts.

---

## Future Improvements / Missing Features
The custom UDP protocol currently implements window flow control, packet tracking, duplicate suppression, and retransmission timeouts. However, the following features are not yet present:
- **Robust Connection Teardown (TIME_WAIT)**: Currently, the sender exits upon receiving the final ACK, and the receiver exits immediately after sending the final ACK. If the final ACK is lost, the sender will timeout and retransmit FIN indefinitely to an exited receiver. A proper TIME_WAIT state is needed.
- **Dynamic Timeout Estimation (RTT/RTO)**: Retransmission timeout is currently fixed (1000ms). Adaptive timeout based on measured Round-Trip Time (RTT) would improve performance.
- **Congestion Control**: The protocol has flow control (window size 10) but lacks congestion control (e.g., Slow Start, Congestion Avoidance) to adapt to network capacity.
- **Fast Retransmit**: Sender only retransmits on timeout. Implementing Fast Retransmit on triple duplicate ACKs would allow quicker recovery from drops.
- **File Metadata Transmission**: Handshake or a metadata block should transmit filename and size so the receiver doesn't hardcode the output to `output.txt`.
- **Application-Level Checksum**: Explicit header checksums (like CRC32) for end-to-end data integrity validation.

### Run this application

**Note**: Using this over a public Wi-Fi network may not work because inbound
UDP packets or device-to-device traffic are often blocked by default.

That being said, you can play around with this application using a personal
Wi-Fi network, a phone hotspot, a VPN such as Tailscale, or two separate
terminal windows on the same host.

#### Build

```bash
make all
```

This creates two executables:

```text
bin/receiver
bin/sender
```

To remove generated binaries and object files:

```bash
make clean
```

#### Run on the same machine

Open two terminal windows from the project root.

In the first terminal, start the receiver:

```bash
./bin/receiver
```

The receiver listens on UDP port `8080` and writes the received file to
`output.txt` in the directory where it was launched.

In the second terminal, send a file:

```bash
./bin/sender tests/large_story.txt
```

When no destination IP is provided, the sender defaults to `127.0.0.1`.

#### Run across two machines

On the receiver machine:

```bash
./bin/receiver
```

The receiver logs the IP address and port that the sender should use. On the
sender machine, pass that IP address explicitly:

```bash
./bin/sender tests/large_story.txt <receiver_ip>
```

For example:

```bash
./bin/sender tests/large_story.txt 192.168.1.23
```

If the sender repeatedly times out while waiting for `SYN-ACK`, first verify
that localhost works, then check that both machines are on a network that allows
device-to-device UDP traffic and that the receiver machine allows inbound UDP on
port `8080`.

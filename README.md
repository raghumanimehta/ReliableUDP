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
- [ ] Add packet sequence numbering
- [ ] Implement basic ACK system
- [ ] Add retransmission mechanism with timeouts
- [ ] Define and document end-of-file signaling

### Testing and Validation
- [ ] Test file transfers under normal conditions
- [ ] Simulate packet loss and validate reliability
- [ ] Log transfer statistics (e.g., number of packets sent, retransmissions)

### 🧰 Stretch Features
- [ ] Basic CLI interface to choose file and mode
- [ ] Web-based or desktop GUI for file selection and status display
- [ ] Dockerize for isolated testing
 
### Handshake protocol 
Compact alternating 3‑way handshake (packet, seq, and state changes):

- **Sender (IDLE → SYN_SENT):** sends `SYN` (seq=0); waits for `SYN-ACK` (retry on timeout - *not implemented yet*).
- **Receiver (IDLE → SYN_RECV → ACK_SENT):** receives `SYN` (seq=0), verifies seq; sends `SYN-ACK` (seq=1); waits for `ACK`.
- **Sender (SYN_SENT → SYN_ACK → ACK_SENT → CONNECTED):** receives `SYN-ACK` (seq=1), verifies seq; sends `ACK` (seq=1) and transitions to `CONNECTED`.
- **Receiver (ACK_SENT → CONNECTED):** receives `ACK` (seq=1), verifies seq; transitions to `CONNECTED` (ready to transfer).

Notes: retries/timeouts and seq verification are applied at each wait step; mismatches trigger retries or aborts.

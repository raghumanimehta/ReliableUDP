# Reliable UDP

## 🎯 Goal
- Implement a reliable file transfer system over **UDP** using custom logic to handle:
  - Packet loss
  - Reordering
  - Duplicate suppression

## 🌟 Stretch Goal
- Develop a simple **web-based** or **desktop UI** to improve usability and allow file transfers via a graphical interface.

---

## ✅ TODO

### 🔧 Core Functionality
- [ ] Set up basic UDP client and server
- [ ] Implement file reading and sending logic on the client
- [ ] Implement file writing logic on the server
- [ ] Add packet sequence numbering
- [ ] Implement basic ACK system
- [ ] Add retransmission mechanism with timeouts
- [ ] Define and document end-of-file signaling

### 🧪 Testing and Validation
- [ ] Test file transfers under normal conditions
- [ ] Simulate packet loss and validate reliability
- [ ] Log transfer statistics (e.g., number of packets sent, retransmissions)

### 🧰 Stretch Features
- [ ] Basic CLI interface to choose file and mode
- [ ] Web-based or desktop GUI for file selection and status display
- [ ] Dockerize for isolated testing

---

Stay tuned for more as the project evolves!

# 🔐 Linux Kernel Keylogger with AVL Tree Password Validation

A Linux kernel module keylogger that captures user keystrokes at the kernel level and stores valid passwords in an AVL tree structure. Designed for **educational and research purposes**, this module demonstrates key kernel programming concepts including:

- Keyboard input notification handling
- Character buffering and parsing
- AVL tree insertion and balancing
- Linux `/proc` file system interaction
- Secure password validation policy

---

## 🛠 Features

- Captures and buffers user keystrokes from the Linux keyboard driver.
- Detects `Shift`, `Enter`, `Backspace`, and special character input.
- Implements a **password policy** that requires at least **3 of the following**:
  - Lowercase letters
  - Uppercase letters
  - Numbers
  - Symbols (`!@#$%^&*()_+-=<>?/.,`)
- Stores **valid passwords** in an **AVL tree** to ensure efficient lookup and balanced insertion.
- Outputs captured valid passwords via a virtual `/proc` file (`/proc/kdb_keylogger`).
- Includes AVL rotations (`LL`, `RR`, `LR`, `RL`) with debug logs.

---

## 🚧 Requirements

- Linux system with kernel headers installed
- Root (superuser) access
- Basic knowledge of building and loading kernel modules

---

## 🧪 Build & Run

### 🔧 Step 1: Compile the Module

```bash
  make



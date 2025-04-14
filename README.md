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
```

### ⚙️ Step 2: Load the Module
```bash
sudo insmod kdb_keylogger.ko
```

### 📖 Step 3: Read Captured Passwords
```bash
cat /proc/kdb_keylogger
```
Only passwords that pass the validation policy are stored and displayed.

### ❌ Step 4: Unload the Module
```bash
sudo rmmod kdb_keylogger
```

---

## 📝 Password Policy
- A password is considered valid if it meets at least three of the following criteria:

- Contains at least one lowercase letter
- Contains at least one uppercase letter
- Contains at least one number
- Contains at least one special character

- Examples of valid passwords:

- A1b!
- abc123$
- Xx99!!

---

## 📦 AVL Tree Design
- Each valid password is stored as a node in the AVL tree.
- The AVL tree is height-balanced using standard rotations (LL, RR, LR, RL).
- All insertions and traversals are logged using printk for debugging.
- The /proc file returns an in-order traversal of the AVL tree.

---

## 📌 Notes
- This module only works on systems that support the legacy keyboard notifier interface (typically real or virtual terminals, not graphical environments).

- Captured keys are stored in a local buffer and only flushed upon detecting Enter or Space.


# Custom File Syatem
 Custom file system in user space
# Custom Filesystem

## Overview

This project implements a custom filesystem in user space. It allows users to perform basic file and directory operations, such as creating, reading, writing, deleting, and listing files. The filesystem is designed using inodes, blocks, and a superblock to manage data, metadata, and storage.

---

## Features

- **File Operations:**
  - Add files.
  - Remove files.
  - Extract files.
  - Open and close files.
- **Directory Operations:**
  - Create directories.
  - List directory contents.
- **Persistence:**
  - Save the filesystem state to a file.
  - Load the filesystem state from a saved file.
- **Custom Data Management:**
  - Uses inodes to manage file metadata and disk blocks for data storage.

---

## Requirements

- **Operating System:** Linux (supports memory mapping with `mmap`).
- **Dependencies:**
  - GCC compiler.

---

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/custom-filesystem.git
   cd custom-filesystem
   ```

2. Compile the project:
   ```bash
   gcc -o filefs filefs.c fs.c
   ```

---

## Usage

### Initialize Filesystem
Create or open a backing file for the filesystem:
```bash
./filefs -f myfs.img
```

### Add a File
Add a file from the host filesystem to the custom filesystem:
```bash
./filefs -a file.txt -f myfs.img
```

### List Files
List all files in the custom filesystem:
```bash
./filefs -l -f myfs.img
```

### Remove a File
Remove a file from the custom filesystem:
```bash
./filefs -r file.txt -f myfs.img
```

### Extract a File
Extract a file from the custom filesystem to the host filesystem:
```bash
./filefs -e file.txt -f myfs.img
```

---

## Example Workflow

1. **Initialize the Filesystem:**
   ```bash
   ./filefs -f myfs.img
   ```

2. **Add a File:**
   ```bash
   ./filefs -a hello.txt -f myfs.img
   ```

3. **List Files:**
   ```bash
   ./filefs -l -f myfs.img
   ```
   Output:
   ```
   Archive myfs.img contains the following files:
   hello.txt
   ```

4. **Extract a File:**
   ```bash
   ./filefs -e hello.txt -f myfs.img
   ```

5. **Remove a File:**
   ```bash
   ./filefs -r hello.txt -f myfs.img
   ```

---

## How It Works

1. **Initialization:**
   - The `formatfs` function creates the superblock, allocates inodes and disk blocks, and sets up a root directory.

2. **File and Directory Operations:**
   - Files and directories are managed using inodes.
   - Data is stored in 512-byte blocks.

3. **Persistence:**
   - The `mysync` function saves the filesystem state to disk.
   - The `loadfs` function restores the filesystem state from disk.

---

## Limitations

- No concurrent access handling.
- Limited maximum file and directory sizes.
- No compression or encryption support.

---

## Contributing

Contributions are welcome! Please fork the repository, make changes, and submit a pull request.

---

## License

This project is licensed under the MIT License. See the LICENSE file for more details.

---

## Contact

- **Author:**Isaac DeVaney	
- **Email:** isaacdevaney@gmail.com 
- **GitHub:**idevane


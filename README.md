# Network File System (NFS)

## Overview
This project implements a **Network File System (NFS)** as part of an [Operating Systems and Networks](https://karthikv1392.github.io/cs3301_osn) course project. The system provides a distributed file storage mechanism where clients can interact with multiple storage servers through a central naming server. The full specification document can be found [here](https://karthikv1392.github.io/cs3301_osn/project/)

## Features
- **Distributed File System**: Supports file operations (read, write, delete, list, create) across multiple storage servers.
- **Naming Server**: Acts as a directory service to locate files in the network.
- **Storage Servers**: Handle actual file storage and retrieval.
- **Client Interactions**: Allows clients to request file operations seamlessly.
- **Audio Streaming**: Supports streaming of audio files over the network.
- **Asynchronous and Synchronous Writes**: Clients can opt for performance-optimized asynchronous writes.
- **Multi-Client Support**: Supports concurrent client access with proper synchronization.
- **LRU Caching**: Implements caching to optimize path lookups in the Naming Server.
- **Failure Handling & Replication**: Data redundancy for reliability.

## Installation & Compilation
### Prerequisites
- **C Compiler (gcc)**
- **Make**
- **POSIX-compatible OS (Linux/macOS)**
- **TCP Networking Support**
- **LibVLC** (for audio streaming)

### Compilation
Run the following command in the project root:
```sh
make
```
This compiles the client, naming server, and storage server binaries.

## Usage
### 1. Start the Naming Server
```sh
./nserver <port>
```
Example:
```sh
./nserver 5000
```

### 2. Start Storage Servers
```sh
./sserver <naming_server_ip> <naming_server_port> <storage_port> <storage_path>
```
Example:
```sh
./sserver 127.0.0.1 5000 6000 /mnt/nfs_storage
```

### 3. Start the Client
```sh
./client <naming_server_ip> <naming_server_port>
```
Example:
```sh
./client 127.0.0.1 5000
```

### 4. Execute File Operations
#### Reading a File
```sh
READ /path/to/file
```
#### Writing to a File
```sh
WRITE /path/to/file "Hello World"
```
#### Streaming an Audio File
```sh
STREAM /path/to/audio.mp3
```

## Architecture Overview
The system consists of three main components:
1. **Naming Server (NM)**: Keeps track of storage servers and file locations.
2. **Storage Servers (SS)**: Store and serve requested files.
3. **Clients**: Interact with the system to perform file operations.

The communication is handled via **TCP sockets**. The Naming Server acts as a mediator between clients and storage servers.

## Assumptions
- Copying a folder merges its contents into the destination rather than nesting it.
- Asynchronous writes assume stdin input is treated as a priority write.
- The system does not support hardcoded IP addresses for distributed testing.

## Contributors
- **Aryan Kumar**
- **Ankith Pai**
- **Akshara Bhatt**
- **Abhiram Tilak**

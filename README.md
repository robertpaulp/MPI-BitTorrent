# **MPI BitTorrent Simulation**

## **Overview**
This project implements a **BitTorrent-like file sharing protocol** using **MPI (Message Passing Interface)**. The goal is to simulate the peer-to-peer file distribution mechanism while ensuring decentralized communication between clients. The project includes multiple components such as a **tracker**, **clients**, and a **peer-based file exchange mechanism**.

## **How BitTorrent Works**
BitTorrent is a peer-to-peer (P2P) communication protocol for distributing files over the internet. Unlike traditional downloads, which rely on a single server, BitTorrent enables users to distribute data among peers, improving speed and efficiency.

### **Key Components:**
- **Tracker**: A server that keeps track of peers sharing a file but does not store the file itself.
- **Seed**: A peer that has a complete copy of the file and only uploads to others.
- **Peer**: A node that downloads and uploads file segments simultaneously.
- **Leecher**: A client that only downloads without sharing segments.

## **Project Logic**

### **Tracker:**
- Manages swarms (mapping files to peers and seeds).
- Provides information about peers and seeds to clients requesting files.
- Updates swarm information as peers complete downloads.

### **Peers:**

- **Initialization:**
   - Each peer reads the file `in<Rank>.txt` to determine the files it owns.
   - The tracker updates `registered_files` and the swarm.

- **File Download:**
   1. First, a request is sent to the tracker to obtain hashes and swarm details.
   2. After the initial setup, the peer starts downloading files.
   3. For the first 10 hashes in the swarm, there are no peers, so all requests are sent to seeds.
   4. **Efficiency:** Once peers start appearing, the strategy follows the recommendation from the project forum:
   - The first 5 segments are sent to peers (if they fail twice, they are sent to seeds).
   - The rest of the segments are sent to seeds.
   5. After completing a file, the peer writes it to disk and sends a message to the tracker, allowing it to become a seed for that file.

- **File Upload:**
   - When receiving a request from a peer (file and hash), the peer checks its seeder list and peer list:
   - If the file exists, it sends the requested hash.
   - If the file does not exist, an error message is sent.
   - Uploading remains active until all download threads have finished and the tracker sends the exit signal.

## **Project Implementation**
This implementation **simulates BitTorrent using MPI**, where each process acts as a tracker or client.

### **Roles in the Simulation:**
- **Tracker (Rank 0 in MPI)**: Manages peer-to-peer communication by maintaining a list of file owners.
- **Clients (Rank > 0 in MPI)**: Download or upload file segments while interacting with other peers and the tracker.
- **Peers** dynamically exchange file segments, avoiding reliance on a central server.

### **Implementation Details**
- The project follows a **distributed model**, where each MPI process can take on different roles.
- **Threading with Pthreads** is used to handle both downloading and uploading simultaneously.
- **File segmentation and exchange** ensure efficient and scalable sharing.

## **Running the Simulation**
### **Compiling the Project**
Run the following command to compile the program:
```sh
make
```
### **Executing the Simulation**
The program is executed using MPI:
```sh
mpirun -np <N> ./mpitorrent
```
where `N` is the number of MPI tasks (must be ≥3).
- **Rank 0** is assigned as the **tracker**.
- **Other processes (Rank 1, 2, ...)** act as **clients**.

### **Input Files**
Each client requires an input file in the format `in<R>.txt`, where `R` is the client rank. The file follows this format:
```
<number_of_owned_files>
<file_name_1> <number_of_segments>
<hash_segment_1>
<hash_segment_2>
...
<number_of_requested_files>
<requested_file_1>
<requested_file_2>
...
```

### **Example Input File (Client 1 - `in1.txt`)**
```
2
fileA 3
abcdef1234567890abcdef1234567890
abcdef9876543210abcdef9876543210
abcdefabcdefabcdefabcdefabcdefabcdef
fileB 2
1234567890abcdef1234567890abcdef
9876543210abcdef9876543210abcdef
1
fileC
```
This means:
- The client **owns two files (`fileA`, `fileB`)**.
- The client **wants to download `fileC`**.
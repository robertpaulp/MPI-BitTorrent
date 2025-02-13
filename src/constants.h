#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <mpi.h>
#include <pthread.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <random>

using namespace std;

// ----- INITIAL -------
#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100

// ----- TRACKER -------
#define TRACKER_COMM_TAG 0
#define TRACKER_FIN_TAG 1

// ----- CLIENT COMMUNICATION TRACKER--------
#define CLIENT_TRACKER_INIT_TAG 5
#define CLIENT_TRACKER_REQ_TAG 6
#define CLIENT_TRACKER_RESEND_TAG 7
#define CLIENT_TRACKER_DWN_FIN_TAG 8

// ----- PEER COMMUNICATION -------
#define CLIENT_UPLOAD_TAG 10
#define PEER_HASH_REQ_TAG 15
#define PEER_HASH_RESP_TAG 16

// ----- MESSAGES -------
#define MESSAGE_SIZE 4
#define INIT_MESSAGE "INT"
#define REQ_MESSAGE "REQ"
#define SWARM_REQ_MESSAGE "SRQ"
#define FIN_FILE_MESSAGE "FFN"
#define CLOSE_MESSAGE "FIN"
#define ACK_MESSAGE "ACK"
#define NOT_FOUND_MESSAGE "NFD"

struct dataClient {
    unordered_map<string, vector<string>> in_files;
    unordered_map<string, vector<string>> desired_files;
};

struct swarmInfo {
    vector<int> seeds;
    vector<int> peers;
};

struct dataTracker {
    unordered_map<string, vector<string>> registered_files;
    unordered_map<string, swarmInfo> swarm;
};

struct FileInfo {
    char filename[MAX_FILENAME];
    int num_segments;
    char segment_hashes[MAX_CHUNKS][HASH_SIZE + 1];
};

struct downloadThreadArgs {
    int rank;
    unordered_map<string, vector<string>> desired_files;
    unordered_map<string, vector<pair<string, bool>>> downloaded_files;
};

struct uploadThreadArgs {
    int rank;
    unordered_map<string, vector<string>> in_files;
    unordered_map<string, vector<pair<string, bool>>> downloaded_files;
};

#endif // CONSTANTS_H
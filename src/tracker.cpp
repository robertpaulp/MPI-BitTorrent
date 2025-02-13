#include "tracker.h"

void client_tracker_req(dataTracker t, MPI_Status status)
{
    // Receive desired files
    int peer_rank = status.MPI_SOURCE;
    int desired_files_count;
    MPI_Recv(&desired_files_count, 1, MPI_INT, peer_rank, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    vector<string> desired_files(desired_files_count);
    for (int i = 0; i < desired_files_count; i++) {
        char filename[MAX_FILENAME] = {0};
        MPI_Recv(filename, MAX_FILENAME, MPI_CHAR, peer_rank, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        desired_files[i] = string(filename);
    }

    for (const auto& file : desired_files) {
        MPI_Send(file.c_str(), MAX_FILENAME, MPI_CHAR, peer_rank, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD);
        
        // Send seeds and peers for desired files
        int seeds_size = t.swarm[file].seeds.size();
        MPI_Send(&seeds_size, 1, MPI_INT, peer_rank, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD);

        for (const auto& seed : t.swarm[file].seeds) {
            MPI_Send(&seed, 1, MPI_INT, peer_rank, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD);
        }

        int peers_size = t.swarm[file].peers.size();
        MPI_Send(&peers_size, 1, MPI_INT, peer_rank, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD);

        for (const auto& peer : t.swarm[file].peers) {
            MPI_Send(&peer, 1, MPI_INT, peer_rank, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD);
        }

        // Send hashes
        int segment_size = t.registered_files[file].size();
        MPI_Send(&segment_size, 1, MPI_INT, peer_rank, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD);

        for (const auto& segment : t.registered_files[file]) {
            char segment_hash[HASH_SIZE] = {0};
            strncpy(segment_hash, segment.c_str(), HASH_SIZE);
            MPI_Send(segment_hash, HASH_SIZE, MPI_CHAR, peer_rank, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD);
        }
    }
}

void resend_swarm_info(dataTracker t, MPI_Status status)
{
    // Receive desired files from peer
    int peer_rank = status.MPI_SOURCE;
    int desired_files_count;
    MPI_Recv(&desired_files_count, 1, MPI_INT, peer_rank, CLIENT_TRACKER_RESEND_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    vector<string> desired_files(desired_files_count);
    for (int i = 0; i < desired_files_count; i++) {
        char filename[MAX_FILENAME] = {0};
        MPI_Recv(filename, MAX_FILENAME, MPI_CHAR, peer_rank, CLIENT_TRACKER_RESEND_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        desired_files[i] = string(filename);
    }

    // Send swarm info for desired files
    for (const auto& file : desired_files) {
        MPI_Send(file.c_str(), MAX_FILENAME, MPI_CHAR, peer_rank, CLIENT_TRACKER_RESEND_TAG, MPI_COMM_WORLD);
        
        int seeds_size = t.swarm[file].seeds.size();
        MPI_Send(&seeds_size, 1, MPI_INT, peer_rank, CLIENT_TRACKER_RESEND_TAG, MPI_COMM_WORLD);

        for (const auto& seed : t.swarm[file].seeds) {
            MPI_Send(&seed, 1, MPI_INT, peer_rank, CLIENT_TRACKER_RESEND_TAG, MPI_COMM_WORLD);
        }

        if (find(t.swarm[file].peers.begin(), t.swarm[file].peers.end(), peer_rank) == t.swarm[file].peers.end()) {
            t.swarm[file].peers.push_back(peer_rank);
        }

        int peers_size = t.swarm[file].peers.size();
        MPI_Send(&peers_size, 1, MPI_INT, peer_rank, CLIENT_TRACKER_RESEND_TAG, MPI_COMM_WORLD);

        for (const auto& peer : t.swarm[file].peers) {
            MPI_Send(&peer, 1, MPI_INT, peer_rank, CLIENT_TRACKER_RESEND_TAG, MPI_COMM_WORLD);
        }
    }
}

void tracker(int numtasks, int rank)
{
    dataTracker t;
    vector<bool> peer_status(numtasks, false);

    // Initialize tracker
    int peer_count = numtasks - 1;

    while (peer_count > 0) {
        MPI_Status status;
        char message[MESSAGE_SIZE] = {0};
        MPI_Recv(message, MESSAGE_SIZE, MPI_CHAR, MPI_ANY_SOURCE, CLIENT_TRACKER_INIT_TAG, MPI_COMM_WORLD, &status);

        if (strcmp(message, INIT_MESSAGE) == 0) {
            int peer_rank = status.MPI_SOURCE;
            int in_files_size;
            MPI_Recv(&in_files_size, 1, MPI_INT, peer_rank, CLIENT_TRACKER_INIT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (int i = 0; i < in_files_size; i++) {
                FileInfo file_info;
                MPI_Recv(&file_info, sizeof(FileInfo), MPI_CHAR, peer_rank, CLIENT_TRACKER_INIT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                t.registered_files[file_info.filename] = vector<string>(file_info.segment_hashes, file_info.segment_hashes + file_info.num_segments);
                t.swarm[file_info.filename].seeds.push_back(peer_rank);
            }

            peer_count--;
        }
    }

    // Send ACK to all peers
    for (int i = 1; i < numtasks; i++) {
        MPI_Send(ACK_MESSAGE, MESSAGE_SIZE, MPI_CHAR, i, CLIENT_TRACKER_INIT_TAG, MPI_COMM_WORLD);
    }

    // cout << "Tracker finished initialization\n";

    int peer_count2 = numtasks - 1;

	while (peer_count2 > 0) {
        MPI_Status status;
        char message[MESSAGE_SIZE] = {0};
        MPI_Recv(message, MESSAGE_SIZE, MPI_CHAR, MPI_ANY_SOURCE, TRACKER_COMM_TAG, MPI_COMM_WORLD, &status);
        
        if (strcmp(message, REQ_MESSAGE) == 0) {
            client_tracker_req(t, status);
        } else if (strcmp(message, SWARM_REQ_MESSAGE) == 0) {    
            resend_swarm_info(t, status);
        } else if (strcmp(message, FIN_FILE_MESSAGE) == 0) {
            char filename[MAX_FILENAME] = {0};
            MPI_Recv(filename, MAX_FILENAME, MPI_CHAR, MPI_ANY_SOURCE, CLIENT_TRACKER_DWN_FIN_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Update swarm if peer finished a file
            auto it = find(t.swarm[filename].peers.begin(), t.swarm[filename].peers.end(), status.MPI_SOURCE);
            if (it == t.swarm[filename].peers.end()) {
                t.swarm[filename].seeds.push_back(status.MPI_SOURCE);
            } else {
                t.swarm[filename].peers.erase(it);
                t.swarm[filename].seeds.push_back(status.MPI_SOURCE);
            }
        } else if (strcmp(message, CLOSE_MESSAGE) == 0) {
            peer_count2--;
        }
    }

    // Send exit signal to all peers
    for (int i = 1; i < numtasks; i++) {
        MPI_Send(CLOSE_MESSAGE, MESSAGE_SIZE, MPI_CHAR, i, CLIENT_UPLOAD_TAG, MPI_COMM_WORLD);
    }
}
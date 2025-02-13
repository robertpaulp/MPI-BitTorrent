#include "peer.h"

// Extract the data from input file
dataClient parse_input_file(int rank) {

    dataClient c;
    unordered_map<string, vector<string>> in_files;

    string filename = "in" + to_string(rank) + ".txt";
    ifstream curr_file(filename);

    if (!curr_file.is_open()) {
        cout << "Error: Couldn't open file " << filename << "\n";
        exit(-1);
    }

    string file_count;
    getline(curr_file, file_count);
    int count = stoi(file_count);

    for (int i = 0; i < count; i++) {
        string file_line;
        getline(curr_file, file_line);

        string file_name = file_line.substr(0, file_line.find(" "));
        int file_seg_count = stoi(file_line.substr(file_line.find(" ") + 1));

        vector<string> segs;
        for (int j = 0; j < file_seg_count; j++) {
            string seg_line;
            getline(curr_file, seg_line);
            segs.push_back(seg_line);
        }

        in_files[file_name] = segs;
    }

    unordered_map<string, vector<string>> desired_files;

    getline(curr_file, file_count);
    int desired_count = stoi(file_count);

    for (int i = 0; i < desired_count; i++) {
        string filename;
        getline(curr_file, filename);
        
        desired_files[filename] = {};
    }

    c.in_files = in_files;
    c.desired_files = desired_files;

    curr_file.close();
    return c;
}

// Write the output file
void write_output_file(int rank, vector<pair<string, bool>> in_hashes, string filename) {
    string outfile = "client" + to_string(rank) + "_" + filename;
    ofstream out_file(outfile, ios::out | ios::trunc);

    if (!out_file.is_open()) {
        cout << "Error: Couldn't open file " << outfile << "\n";
        exit(-1);
    }

    for (size_t i = 0; i < in_hashes.size(); i++) {
        if (i == in_hashes.size() - 1) {
            out_file << in_hashes[i].first;
        } else {
            out_file << in_hashes[i].first << "\n";
        }
    }

    out_file.close();
}

void send_request_to_tracker(unordered_map<string, vector<string>> desired_files)
{
    // Send request message "REQ"
    MPI_Send(REQ_MESSAGE, MESSAGE_SIZE, MPI_CHAR, TRACKER_RANK, TRACKER_COMM_TAG, MPI_COMM_WORLD);

    int desired_files_count = desired_files.size();
    MPI_Send(&desired_files_count, 1, MPI_INT, TRACKER_RANK, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD);

    for (auto& file : desired_files) {
        string filename = file.first;
        MPI_Send(filename.c_str(), MAX_FILENAME, MPI_CHAR, TRACKER_RANK, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD);
    }
}

void receive_swarm_and_file_info(unordered_map<string, swarmInfo>& swarm_info, unordered_map<string, vector<pair<string, bool>>>& downloaded_files, int desired_files_count)
{
    for (int i = 0; i < desired_files_count; i++) {
        char filename[MAX_FILENAME] = {0};
        MPI_Recv(filename, MAX_FILENAME, MPI_CHAR, TRACKER_RANK, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Receive swarm info
        int seeds_size;
        MPI_Recv(&seeds_size, 1, MPI_INT, TRACKER_RANK, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        vector<int> seeds(seeds_size);
        for (int i = 0; i < seeds_size; i++) {
            MPI_Recv(&seeds[i], 1, MPI_INT, TRACKER_RANK, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        int peers_size;
        MPI_Recv(&peers_size, 1, MPI_INT, TRACKER_RANK, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        vector<int> peers(peers_size);
        for (int i = 0; i < peers_size; i++) {
            MPI_Recv(&peers[i], 1, MPI_INT, TRACKER_RANK, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Receive file info
        int segment_size;
        MPI_Recv(&segment_size, 1, MPI_INT, TRACKER_RANK, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        vector<pair<string, bool>> segments(segment_size);
        for (int i = 0; i < segment_size; i++) {
            char segment_hash[HASH_SIZE + 1] = {0};
            MPI_Recv(segment_hash, HASH_SIZE, MPI_CHAR, TRACKER_RANK, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            segments[i] = {string(segment_hash), false};
        }

        swarm_info[string(filename)] = {seeds, peers};
        downloaded_files[string(filename)] = segments;
    }
}

void update_swarm_info(unordered_map<string, swarmInfo>& swarm_info, vector<string> desired_files, int rank)
{
    // Send request message "SRQ"
    MPI_Send(SWARM_REQ_MESSAGE, MESSAGE_SIZE, MPI_CHAR, TRACKER_RANK, TRACKER_COMM_TAG, MPI_COMM_WORLD);

    int desired_files_count = desired_files.size();
    MPI_Send(&desired_files_count, 1, MPI_INT, TRACKER_RANK, CLIENT_TRACKER_RESEND_TAG, MPI_COMM_WORLD);

    for (auto& file : desired_files) {
        string filename = file;
        MPI_Send(filename.c_str(), MAX_FILENAME, MPI_CHAR, TRACKER_RANK, CLIENT_TRACKER_RESEND_TAG, MPI_COMM_WORLD);
    }

    // Receive the new seeds and peers
    for (int i = 0; i < desired_files_count; i++) {
        char filename[MAX_FILENAME] = {0};
        MPI_Recv(filename, MAX_FILENAME, MPI_CHAR, TRACKER_RANK, CLIENT_TRACKER_RESEND_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        int seeds_size;
        MPI_Recv(&seeds_size, 1, MPI_INT, TRACKER_RANK, CLIENT_TRACKER_RESEND_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        vector<int> seeds(seeds_size);
        for (int i = 0; i < seeds_size; i++) {
            MPI_Recv(&seeds[i], 1, MPI_INT, TRACKER_RANK, CLIENT_TRACKER_RESEND_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        int peers_size;
        MPI_Recv(&peers_size, 1, MPI_INT, TRACKER_RANK, CLIENT_TRACKER_RESEND_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        vector<int> peers(peers_size);
        for (int i = 0; i < peers_size; i++) {
            MPI_Recv(&peers[i], 1, MPI_INT, TRACKER_RANK, CLIENT_TRACKER_RESEND_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        swarm_info[string(filename)] = {seeds, peers};
    }
}

void process_download(int rank, string filename, string hash, int first_peer, unordered_map<string, swarmInfo>& swarm_info, unordered_map<string, vector<pair<string, bool>>>& downloaded_files, int& seg_count, bool& downloaded)
{
    // Send request message "REQ"
    MPI_Send(REQ_MESSAGE, MESSAGE_SIZE, MPI_CHAR, first_peer, CLIENT_UPLOAD_TAG, MPI_COMM_WORLD);

    MPI_Send(filename.c_str(), MAX_FILENAME, MPI_CHAR, first_peer, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD);
    MPI_Send(hash.c_str(), HASH_SIZE, MPI_CHAR, first_peer, PEER_HASH_REQ_TAG, MPI_COMM_WORLD);

    char received_hash[HASH_SIZE + 1] = {0};
    MPI_Recv(received_hash, HASH_SIZE, MPI_CHAR, first_peer, PEER_HASH_RESP_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if (strcmp(hash.c_str(), received_hash) == 0) {
        downloaded = true;
        seg_count++;
    }

    // Update swarm after 10 segments
    if (seg_count % 10 == 0 && seg_count != 0) {
        vector<string> desired_files;
        for (auto& [filename, segments] : downloaded_files) {
            desired_files.push_back(filename);
        }
        update_swarm_info(swarm_info, desired_files, rank);
    }
}

void *download_thread_func(void *arg)
{
    struct downloadThreadArgs *args = (struct downloadThreadArgs *) arg;
    int rank = args->rank;
    unordered_map<string, vector<string>> desired_files = args->desired_files;
    unordered_map<string, vector<pair<string, bool>>> downloaded_files = args->downloaded_files;
    
    unordered_map<string, swarmInfo> swarm_info;

    send_request_to_tracker(desired_files);
    receive_swarm_and_file_info(swarm_info, downloaded_files, desired_files.size());


    // Download files
    int seg_count = 0;
    for (auto& [filename, segments] : downloaded_files) {
        for (auto& [hash, downloaded] : segments) {
            int last_digit_seg = seg_count % 10;

            while(downloaded == false) {
                if (last_digit_seg < 5 && swarm_info[filename].peers.size() > 0) {
                    int first_peer = -1;
                    int peer_fail_count = 0;
                    bool was_downloaded = false;

                    while (was_downloaded == false) {
                        random_device rd;
                        mt19937 gen(rd());
                        uniform_int_distribution<> distr(0, swarm_info[filename].peers.size() - 1);

                        // Try a random peer for two times, if it still fails, download from seed
                        if (peer_fail_count < 2 && swarm_info[filename].peers.size() > 1) {
                            do {
                                first_peer = swarm_info[filename].peers[distr(gen)];
                            } while (first_peer == rank);
                        } else {
                            first_peer = swarm_info[filename].seeds[0];
                        }

                        process_download(rank, filename, hash, first_peer, swarm_info, downloaded_files, seg_count, was_downloaded);

                        if (was_downloaded) {
                            downloaded = true;
                        } else {
                            peer_fail_count++;
                        }
                    }
                } else {
                    int first_peer = -1;
                    random_device rd;
                    mt19937 gen(rd());
                    uniform_int_distribution<> distr(0, swarm_info[filename].seeds.size() - 1);

                    do {
                        first_peer = swarm_info[filename].seeds[distr(gen)];
                    } while (first_peer == rank);

                    bool was_downloaded = false;
                    process_download(rank, filename, hash, first_peer, swarm_info, downloaded_files, seg_count, was_downloaded);

                    if (was_downloaded) {
                        downloaded = true;
                    }
                }
            }
        }

        // Write output curr_file
        write_output_file(rank, segments, filename);

        // Notify tracker that peer has finished downloading the curr_file
        MPI_Send(FIN_FILE_MESSAGE, MESSAGE_SIZE, MPI_CHAR, TRACKER_RANK, TRACKER_COMM_TAG, MPI_COMM_WORLD);
        MPI_Send(filename.c_str(), MAX_FILENAME, MPI_CHAR, TRACKER_RANK, CLIENT_TRACKER_DWN_FIN_TAG, MPI_COMM_WORLD);
    }
    
    // Notify tracker that peer has finished downloading all
    MPI_Send(CLOSE_MESSAGE, MESSAGE_SIZE, MPI_CHAR, TRACKER_RANK, TRACKER_COMM_TAG, MPI_COMM_WORLD);

    return NULL;
}

void *upload_thread_func(void *arg)
{
    struct uploadThreadArgs *args = (struct uploadThreadArgs *) arg;
    int rank = args->rank;
    unordered_map<string, vector<string>> in_files = args->in_files;
    unordered_map<string, vector<pair<string, bool>>> downloaded_files = args->downloaded_files;

    bool is_running = true;
    char message[MESSAGE_SIZE] = {0};

    while(is_running) {
        MPI_Status status;
        MPI_Recv(message, MESSAGE_SIZE, MPI_CHAR, MPI_ANY_SOURCE, CLIENT_UPLOAD_TAG, MPI_COMM_WORLD, &status);

        if(strcmp(message, CLOSE_MESSAGE) == 0) {
            is_running = false;
        } else if(strcmp(message, REQ_MESSAGE) == 0) {
            int peer_rank = status.MPI_SOURCE;
            char filename[MAX_FILENAME] = {0};
            char hash[HASH_SIZE + 1] = {0};
            
            // Receive file name and hash
            MPI_Recv(filename, MAX_FILENAME, MPI_CHAR, peer_rank, CLIENT_TRACKER_REQ_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(hash, HASH_SIZE, MPI_CHAR, peer_rank, PEER_HASH_REQ_TAG, MPI_COMM_WORLD, &status);
            
            string file = string(filename);
            string segment_hash = string(hash);
            bool found = false;

            // Look for segment in seed files, then in peer files
            if (in_files.find(file) != in_files.end()) {
                for (auto &seg : in_files[file]) {
                    if (seg == segment_hash) {
                        found = true;
                        break;
                    }
                }
            }
            if (!found && downloaded_files.find(file) != downloaded_files.end()) {
                for (auto &seg : downloaded_files[file]) {
                    if (seg.first == segment_hash) {
                        found = true;
                        break;
                    }
                }
            }

            if (found) {
                MPI_Send(hash, HASH_SIZE, MPI_CHAR, peer_rank, PEER_HASH_RESP_TAG, MPI_COMM_WORLD);
            } else {
                MPI_Send(NOT_FOUND_MESSAGE, MESSAGE_SIZE, MPI_CHAR, peer_rank, PEER_HASH_RESP_TAG, MPI_COMM_WORLD);
            }
        }
    }
    return NULL;
}

void initialize_tracker_conn(dataClient c)
{    
    // Send init message "INT"
    MPI_Send(INIT_MESSAGE, MESSAGE_SIZE, MPI_CHAR, TRACKER_RANK, CLIENT_TRACKER_INIT_TAG, MPI_COMM_WORLD);

    // Sending info about files and their segments
    int file_count = c.in_files.size();
    MPI_Send(&file_count, 1, MPI_INT, TRACKER_RANK, CLIENT_TRACKER_INIT_TAG, MPI_COMM_WORLD);

    for (auto& [filename, segments] : c.in_files) {
        FileInfo file_info;
        strncpy(file_info.filename, filename.c_str(), MAX_FILENAME);
        file_info.num_segments = segments.size();

        for (int i = 0; i < (int) segments.size(); i++) {
            strncpy(file_info.segment_hashes[i], segments[i].c_str(), HASH_SIZE);
        }

        MPI_Send(&file_info, sizeof(FileInfo), MPI_CHAR, TRACKER_RANK, CLIENT_TRACKER_INIT_TAG, MPI_COMM_WORLD);
    }

    // Wait for tracker response
    char message[MESSAGE_SIZE] = {0};
    MPI_Recv(message, MESSAGE_SIZE, MPI_CHAR, TRACKER_RANK, CLIENT_TRACKER_INIT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

void peer(int numtasks, int rank) {
    pthread_t download_thread;
    pthread_t upload_thread;
    void *status;
    int r;

    // Read input file
    dataClient c = parse_input_file(rank);

    // Initialize connection with tracker
    initialize_tracker_conn(c);

    // Start download and upload threads
    unordered_map<string, vector<pair<string, bool>>> download_progress;
    struct downloadThreadArgs download_args = {rank, c.desired_files, download_progress};
    struct uploadThreadArgs upload_args = {rank, c.in_files, download_progress};

    r = pthread_create(&download_thread, NULL, download_thread_func, (void *) &download_args);
    if (r) {
        printf("Eroare la crearea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_create(&upload_thread, NULL, upload_thread_func, (void *) &upload_args);
    if (r) {
        printf("Eroare la crearea thread-ului de upload\n");
        exit(-1);
    }

    r = pthread_join(download_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_join(upload_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de upload\n");
        exit(-1);
    }
}
 
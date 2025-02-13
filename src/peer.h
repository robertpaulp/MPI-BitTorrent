#ifndef PEER_H
#define PEER_H

#include "constants.h"

dataClient parse_input_file(int rank);
void write_output_file(int rank, vector<pair<string, bool>> in_hashes, string filename);

void send_request_to_tracker(unordered_map<string, vector<string>> desired_files);
void receive_swarm_and_file_info(unordered_map<string, swarmInfo>& swarm_info, unordered_map<string, vector<pair<string, bool>>>& downloaded_files, int desired_files_count);
void update_swarm_info(unordered_map<string, swarmInfo>& swarm_info, vector<string> desired_files, int rank);
void process_download(int rank, string filename, string hash, int first_peer, unordered_map<string, swarmInfo>& swarm_info, unordered_map<string, vector<pair<string, bool>>>& downloaded_files, int& seg_count, bool& downloaded);
void *download_thread_func(void *args);

void *upload_thread_func(void *args);

void initialize_tracker_conn(dataClient c);
void peer(int numtasks, int rank);

#endif // PEER_H
#ifndef TRACKER_H
#define TRACKER_H

#include "constants.h"

void tracker(int numtasks, int rank);
void client_tracker_req(dataTracker t, MPI_Status status);
void resend_swarm_info(dataTracker t, MPI_Status status);

#endif // TRACKER_H

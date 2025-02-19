#include "scheduler.h"
#include "common.h"
#include <queue>

void simulate_rr(
    int64_t quantum, int64_t max_seq_len, std::vector<Process> & processes, std::vector<int> & seq)
{
    seq.clear();

    if (processes.size() < 1) return;

    int64_t curr_time = 0;
    // the time remaining in the quantum for a proc on the cpu
    int64_t time_rem_in_tslice = quantum;
    std::queue<int> rq;
    // vector that keeps track of the remaining burst for each process
    std::vector<int64_t> proc_rem_bursts;
    uint64_t jobs_remaining = processes.size();

    // Note -1 => cpu is idle
    int64_t proc_on_cpu = -1;

    if (processes[0].arrival_time > curr_time && (int64_t)seq.size() < max_seq_len)
        seq.push_back(-1);

    // Loop through all the processes and set start time to -1 this flag is useful to know if a proc
    // has start running on the cpu
    for (auto & process : processes) {
        process.start_time = -1;
        process.finish_time = -1;
    }

    curr_time += processes[0].arrival_time;
    processes[0].start_time = curr_time;
    if ((int64_t)seq.size() < max_seq_len) seq.push_back(processes[0].id);

    proc_on_cpu = processes[0].id;
    proc_rem_bursts.push_back(processes[0].burst);
    // iterator to go through each process and keep track if i have gone through them all
    std::vector<Process>::iterator it = processes.begin();
    it++;

    while (1) {
        // If all jobs are done quit out of fnc
        if (jobs_remaining == 0) break;
        // Process on the cpu is done
        if (proc_on_cpu != -1 && proc_rem_bursts[proc_on_cpu] == 0) {
            processes[proc_on_cpu].finish_time = curr_time;
            // set to -1 so I know that this a burst I dont have to worry about anymore
            proc_rem_bursts[proc_on_cpu] = -1;
            proc_on_cpu = -1; // cpu is idle
            jobs_remaining--;
            continue;
        }

        // if there is a remaining job and its the very last proc skip time and break
        if (jobs_remaining == 1 && rq.empty() && it == processes.end() && proc_on_cpu != -1) {
            processes[proc_on_cpu].finish_time = proc_rem_bursts[proc_on_cpu] + curr_time;
            jobs_remaining--;
            break;
        }
        // exceeded quantum
        if (time_rem_in_tslice == 0 && proc_on_cpu != -1) {
            // put back in RQ
            rq.push(proc_on_cpu);
            proc_on_cpu = -1;
            continue;
        }

        // check for a new process
        if (it != processes.end() && (curr_time == it->arrival_time)) {
            rq.push(it->id);
            it++;
            continue;
        }

        // cpu idle & rq not empty
        if (proc_on_cpu == -1 && (! rq.empty())) {
            // put the next thing from rq on the cpu
            proc_on_cpu = rq.front();
            rq.pop();
            // check if it has started and give it a start time
            if (processes[proc_on_cpu].start_time == -1) {
                processes[proc_on_cpu].start_time = curr_time;
                proc_rem_bursts.push_back(processes[proc_on_cpu].burst);
            }
            if ((int64_t)seq.size() < max_seq_len) {
                if (seq.back() != proc_on_cpu) seq.push_back(proc_on_cpu);
            }
            time_rem_in_tslice = quantum;
            continue;
        }
        // RQ empty and cpu is idle => jobs_remaining > 0 and a process hasn't moved from job queue
        if (rq.empty() && proc_on_cpu == -1) {
            curr_time = it->arrival_time;
            if ((int64_t)seq.size() < max_seq_len) {
                if (seq.back() != proc_on_cpu) seq.push_back(proc_on_cpu);
            }
            continue;
        }

        //  RQ empty and cpu has a job
        if (rq.empty() && proc_on_cpu != -1 && proc_rem_bursts[proc_on_cpu] != 0
            && it != processes.end()) {
            int64_t safe_jumps;
            safe_jumps = (it->arrival_time - curr_time) / (quantum);
            proc_rem_bursts[proc_on_cpu] = proc_rem_bursts[proc_on_cpu] - (quantum * safe_jumps);
            curr_time += (quantum * safe_jumps);
        }

        // when CPU is idle, but RQ is not empty, and there are jobs still arriving in the future
        if (! rq.empty() && proc_on_cpu != -1 && proc_rem_bursts[proc_on_cpu] != 0) {
            // check to see if every job on the rq has started
            bool has_started = true;
            for (size_t i = 0; i < rq.size(); i++) {
                int64_t temp = rq.front();
                rq.pop();
                if (processes[temp].start_time == -1) has_started = false;
                rq.push(temp);
            }
            if (has_started) {
                // Find the minimum non-zero burst in the RQ
                int64_t min_burst = INT64_MAX;
                for (size_t i = 0; i < proc_rem_bursts.size(); i++) {

                    if ((proc_rem_bursts[i] < min_burst) && proc_rem_bursts[i] > 0)
                        min_burst = proc_rem_bursts[i];
                }

                // check to see if the min is less than the quantum if its not then you just want to
                // continue on as normal
                if (min_burst > quantum) {
                    // Divide the min-burst by the quantum and take the floor. N is the multiple you
                    // can saftely skip
                    int64_t N = min_burst / quantum;

                    // check what the limiting factor is min_burst or arrival time of the next job
                    if (it != processes.end()
                        && (N * quantum * ((int64_t)rq.size() + 1) + curr_time
                            > it->arrival_time)) {
                        // rq.size() + 1 to account for the fact that there is already a job on the
                        // cpu
                        N = (it->arrival_time - curr_time) / (quantum * (rq.size() + 1));
                    } else
                        N -= 1;

                    if (N > 0) {
                        // safe_jumps is the current time you can skip, so you need to divide this
                        // by rq.size() +1 to find out how many times each burst should skip by
                        int64_t safe_jumps = N * (rq.size() + 1) * quantum;
                        curr_time += safe_jumps;
                        for (auto & burst : proc_rem_bursts) {
                            burst -= (safe_jumps / (rq.size() + 1));
                        }
                        // Adding the processes to the sequence
                        for (int64_t i = 0; i < N; i++) {
                            if ((int64_t)seq.size() < max_seq_len) {
                                if (seq.back() != proc_on_cpu) seq.push_back(proc_on_cpu);
                            } else {
                                break;
                            }
                            std::queue<int> tempq = rq;
                            for (size_t j = 0; j < rq.size(); j++) {
                                if ((int64_t)seq.size() < max_seq_len) {
                                    if ((seq.back() != tempq.front())) {
                                        int temp_proc = tempq.front();
                                        tempq.pop();
                                        seq.push_back(temp_proc);
                                        tempq.push(temp_proc);
                                    }
                                } else
                                    break;
                            }
                        }
                        // to account for the last curr proc that the loop above didnt handle
                        if ((int64_t)seq.size() < max_seq_len) {
                            if (seq.back() != proc_on_cpu) seq.push_back(proc_on_cpu);
                        }
                    }
                }
            }
        }

        // fast forward the time
        int64_t inc = INT64_MAX;
        if (it != processes.end()) inc = it->arrival_time - curr_time;

        // cpu not idle
        if (proc_on_cpu != -1) {
            if (time_rem_in_tslice < inc) inc = time_rem_in_tslice;
            if (proc_rem_bursts[proc_on_cpu] < inc) inc = proc_rem_bursts[proc_on_cpu];

            proc_rem_bursts[proc_on_cpu] -= inc;
            time_rem_in_tslice -= inc;
        }
        curr_time += inc;
    }
}
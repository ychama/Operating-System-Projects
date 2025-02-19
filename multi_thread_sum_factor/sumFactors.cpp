#include "sumFactors.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <pthread.h>
#include <atomic>
#include <algorithm>

struct Task
{
  pthread_t tid;
  int64_t start, end, orignal_number;
} tasks[256];

bool global_finished;
/*Used to check if the number read from the file is (2,3 => prime) or a factor of 2 or 3
If true then the thread running the serial task exits the serial task func and moves onto the next number 
*/
bool initial_check;
//The global variable used to sum up the smallest factor of each number
int64_t global_sum;
pthread_barrier_t barr_id;
int64_t N_THREADS;

//Tries to update the min value based on an already known value atomically
template <typename T>
void update_min(std::atomic<T> &av, T const &v) noexcept
{
  T prev = av;
  while (prev > v && !av.compare_exchange_weak(prev, v))
    ;
}
std::atomic<int64_t> min_divisor_found{std::numeric_limits<int64_t>::max()};

//This parallel task tries to find the smallest factor in the range assigned to a thread
void *parallel_task(void *threadTask)
{
  Task *tTask = (Task *)threadTask;
  int64_t i = tTask->start;
  int64_t max = tTask->end;
  int64_t number = tTask->orignal_number;

  while (i <= max)
  {
    if (min_divisor_found.load() <= max && min_divisor_found.load() != std::numeric_limits<int64_t>::max())
      return 0;

    if (number % i == 0 && number != i)
    {
      update_min(min_divisor_found, i);
    }
    if (number % (i + 2) == 0 && number != (i + 2))
    {
      update_min(min_divisor_found, ((i + 2)));
    }
    i += 6;
  }
  return 0;
}

//This reads the number from the text file and splits up the work among the threads equally
void serial_task()
{
  //update global sum with min factor
  if (min_divisor_found.load() != std::numeric_limits<int64_t>::max())
  {
    global_sum += min_divisor_found.load();
  }

  initial_check = false;
  min_divisor_found = std::numeric_limits<int64_t>::max();

  int64_t num;
  if (!(std::cin >> num))
  {
    global_finished = true;
    return;
  }

  //split up the work
  if (num <= 3)
  {
    global_sum += 0; // 2 and 3 are primes
    initial_check = true;
    return;
  }

  else if (num % 2 == 0)
  {
    global_sum += 2;
    initial_check = true;
    return;
  } // handle multiples of 2
  if (num % 3 == 0)
  {
    global_sum += 3; // handle multiples of 3
    initial_check = true;
    return;
  }

  int64_t start = 5;
  int64_t max = sqrt(num);
  int64_t diff = max - start;
  int64_t split = diff / (6 * N_THREADS);

  for (int64_t i = 0; i < N_THREADS; i++)
  {
    int64_t ts = start;
    int64_t te = ts + (6 * split);
    if (i == (N_THREADS - 1))
      te += (max - te);
    else
      start = te;

    tasks[i].start = ts;
    tasks[i].end = te;
    tasks[i].orignal_number = num;
  }
  return;
}
// This is my implementation of the fork-join pbarrier model
void *thread_start(void *threadTask)
{
  struct Task *tTask = (struct Task *)threadTask;

  while (1)
  {
    //serial task first
    int res = pthread_barrier_wait(&barr_id);
    if (res == PTHREAD_BARRIER_SERIAL_THREAD)
    {
      serial_task();
    }
    pthread_barrier_wait(&barr_id);
    if (global_finished)
      pthread_exit(0);

    if (initial_check)
      continue;

    parallel_task(tTask);
    pthread_barrier_wait(&barr_id);
  }
}
// This function is used if the # of threads is 1
int64_t get_smallest_divisor(int64_t n)
{
  if (n <= 3)
    return 0; // 2 and 3 are primes
  if (n % 2 == 0)
    return 2; // handle multiples of 2
  if (n % 3 == 0)
    return 3; // handle multiples of 3
  int64_t i = 5;
  int64_t max = sqrt(n);
  while (i <= max)
  {
    if (n % i == 0)
      return i;
    if (n % (i + 2) == 0)
      return i + 2;
    i += 6;
  }
  return 0;
}

int64_t sum_factors(int n_threads)
{
  N_THREADS = n_threads;

  if (N_THREADS == 1)
  {
    int64_t sum = 0;
    while (1)
    {
      int64_t num;
      if (!(std::cin >> num))
        break;
      int64_t div = get_smallest_divisor(num);
      sum += div;
    }
  }

  pthread_barrier_init(&barr_id, NULL, N_THREADS);

  for (int i = 0; i < N_THREADS; i++)
  {
    if (0 != pthread_create(&tasks[i].tid, 0, thread_start, &tasks[i]))
    {
      printf("Error: pthread_creatre failed\n");
      exit(-1);
    }
  }
  global_finished = false;
  initial_check = false;
  global_sum = 0;
  min_divisor_found = std::numeric_limits<int64_t>::max();

  for (int i = 0; i < N_THREADS; i++)
    pthread_join(tasks[i].tid, 0);

  pthread_barrier_destroy(&barr_id);

  return global_sum;
}

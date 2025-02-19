#include "fatsim.h"
#include <cstdio>
#include <stack>

void fat_sim(const std::vector<long> & fat, long & longest_file_blocks, long & unused_blocks)
{
  std::vector<bool> visited(fat.size(), false);
  //-1 => cycle,  0 => nullptr >0 => seq. I start with -1 assuming everything is in a cycle

  std::vector<long> sequence(fat.size(), -1);
  std::stack<long> dfs;

  longest_file_blocks = 0;
  // Start by setting everything thats a deadend (ie. nullptr) to visited
  for (size_t i = 0; i < fat.size(); i++)
    if (fat[i] == -1) {
      visited[i] = true;
      sequence[i] = 0;
    }

  for (size_t i = 0; i < fat.size(); i++) {
    if (visited[i]) continue;
    dfs.push(i);
    while (! visited[dfs.top()]) {
      visited[dfs.top()] = true;
      dfs.push(fat[dfs.top()]);
    }
      // If the next thing in the seq is a dead end unwind and update the seq
    if(fat[dfs.top()] == -1){
      long count = 0;
      while(!dfs.empty()){
        sequence[dfs.top()] = count +1;
        dfs.pop();
        count ++;
      }
    }
  
     // Otherwise I check to see if the next thing has a seq and increment and unwind the stack
    else {
      while (! dfs.empty()) {
        if (sequence[fat[dfs.top()]] > 0) sequence[dfs.top()] = sequence[fat[dfs.top()]] + 1;
        dfs.pop();
      }
    }
  }
  // Update the required values
  for (size_t i = 0; i < sequence.size(); i++) {
    if (sequence[i] > longest_file_blocks) longest_file_blocks = sequence[i];
    else if (sequence[i] == -1)
      unused_blocks++;
  }
}
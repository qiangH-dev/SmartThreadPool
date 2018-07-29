/****************************************************************************\
 * Created on Sun Jul 29 2018
 * 
 * The MIT License (MIT)
 * Copyright (c) 2018 leosocy
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the ",Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED ",AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
\*****************************************************************************/

#include "smart_thread_pool.h"
#include "gtest/gtest.h"
#include <thread>
#include <chrono>

namespace {

using stp::TaskPriority;
using stp::TaskQueue;

class TaskQueueTestFixture : public testing::Test {
 protected:
  TaskQueue default_queue_;
//   TaskQueue low_queue_;
//   TaskQueue medium_queue_;
//   TaskQueue high_queue_;
//   TaskQueue urgent_queue_;
  TaskQueueTestFixture()
    : default_queue_(){}
  virtual void SetUp() {
    default_queue_ = TaskQueue();
    // low_queue_ = TaskQueue(TaskPriority::LOW);
    // medium_queue_ = TaskQueue(TaskPriority::MEDIUM);
    // high_queue_ = TaskQueue(TaskPriority::HIGH);
    // urgent_queue_ = TaskQueue(TaskPriority::URGENT);
  }
  virtual void TearDown() {
    default_queue_.ClearQueue();
  }
};

TEST_F(TaskQueueTestFixture, test_multithread_produce_task_but_no_worker_consume) {
  auto task_func = [](){};
  auto entry = [this, task_func](){
    auto res = default_queue_.enqueue(task_func);
  };
  std::thread ts[16];
  for (int i = 0; i < 16; ++i) {
    ts[i] = std::thread(entry);
  }
  for (int i = 0; i < 16; ++i) {
    ts[i].join();
  }
  
  EXPECT_EQ(default_queue_.size(), 16);
}

TEST_F(TaskQueueTestFixture, test_multithread_consume_task_but_no_task) {
  std::mutex mtx;
  int run_count = 0;
  auto task_func = [&run_count, &mtx]() {
    std::unique_lock<std::mutex> lock(mtx);
    run_count += 1;
  };
  auto entry = [this](){
    auto task = default_queue_.dequeue();
    if (task) {
      (*task)();
    }
  };
  auto clear_queue_entry = [this]() {
    default_queue_.ClearQueue();
  };
  std::thread ts[16];
  for (int i = 0; i < 15; ++i) {
    ts[i] = std::thread(entry);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ts[15] = std::thread(clear_queue_entry);
  for (int i = 0; i < 16; ++i) {
    ts[i].join();
  }

  EXPECT_EQ(run_count, 0);
  EXPECT_EQ(default_queue_.size(), 0);
}

TEST_F(TaskQueueTestFixture, test_multithread_run_task) {
  std::mutex mtx;
  int run_count = 0;
  auto task_func = [&run_count, &mtx](int count) {
    std::unique_lock<std::mutex> lock(mtx);
    run_count += 1;
    lock.unlock();
    return count;
  };
  auto entry = [this, task_func](int count){
    auto res = default_queue_.enqueue(task_func, count);
    auto task = default_queue_.dequeue();
    if (task) {
      (*task)();
    }
    EXPECT_EQ(res.get(), count);
  };
  std::thread ts[16];
  for (int i = 0; i < 16; ++i) {
    ts[i] = std::thread(entry, i);
  }
  for (int i = 0; i < 16; ++i) {
    ts[i].join();
  }

  EXPECT_EQ(run_count, 16);
  EXPECT_EQ(default_queue_.size(), 0);
}

}   // namespace
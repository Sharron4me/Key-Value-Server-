//	Reference: Thread Pool Tutorial - How-To (Youtube), https://github.com/mtrebi/thread-pool

#pragma once
#include <iostream>
#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <exception>
#include <future>
#include <mutex>
#include <unistd.h>
#include <queue>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>

class threadpool {

public:

	using job = std::function<void()>;

	explicit threadpool(std::size_t nthreads){
		num_threads = nthreads;
		start(num_threads);
	}

	template<typename F, typename...Args>
	auto enqueue(F&& new_job, Args&&... args) -> std::future<decltype(new_job(args...))> {
		{
			std::unique_lock<std::mutex> lock(event_lock);
			std::function<decltype(new_job(args...))()> func = std::bind(std::forward<F>(new_job), std::forward<Args>(args)...);
			auto task_ptr = std::make_shared<std::packaged_task<decltype(new_job(args...))()>>(func);
			std::function<void()> wrapper_func = [task_ptr]() {
				(*task_ptr)();
			};
			jobs.emplace(wrapper_func);
		}
		event_notifier.notify_one();
	}

	std::size_t num_threads;


private:
	std::mutex event_lock;
	bool threadstop = false;
	std::vector<std::thread> threads;
	std::condition_variable event_notifier;
	std::queue<job> jobs;

	void start(std::size_t num_threads){
		for (int i = 0; i < num_threads; i++) {
			threads.emplace_back([&] {
				job job_to_be_executed;
				while(true) {
//					get lock as critical section scope entered
					{
						std::unique_lock<std::mutex> lock(event_lock);
						event_notifier.wait(lock);
						if (threadstop) {
							break;
						}
						if (!jobs.empty()) {
							job_to_be_executed = std::move(jobs.front());
							jobs.pop();
						}
					}
//					Release lock as critical section scope is over
					job_to_be_executed();
				}
			});
		}
	}

	
	
};

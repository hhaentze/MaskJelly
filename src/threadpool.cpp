	#include "threadpool.hpp"
	
	using namespace std;

	ThreadPool::ThreadPool (size_t nr_threads) 
		: nr_threads (nr_threads),
		finished (false),
		threads (nr_threads),
		nr_active_threads(0)
	{
		for (auto& t: this->threads) {
			t = thread([=](){process_jobs();});
		}
	}

	ThreadPool::~ThreadPool () {
		{
			unique_lock<mutex> lock(this->m);
			this->finished = true;
		}
		this->cv.notify_all();
		for (auto& t : this->threads) {
			t.join();
		}
	}

	void ThreadPool::submit (Job job) {
		unique_lock<mutex> lock(this->m);
		this->jobs.push_back(move(job));
		this->cv.notify_one();
	}

	void ThreadPool::process_jobs () {
		for (;;) {
			Job job;
			{
				unique_lock<mutex> lock(this->m);
				while (this->jobs.empty() && !this->finished) {
					this->cv.wait(lock);
				}
				if (this->jobs.empty() && this->finished) {
					break;
				}
				job = move(this->jobs.front());
				this->jobs.pop_front();
			}
			this->nr_active_threads++;
			job();
			this->nr_active_threads--;
			cv.notify_all();
		}
	}

	void ThreadPool::waitFinished(){
		unique_lock<mutex> lock(this->m);
		this->cv.wait(lock,[this](){return this->jobs.empty() && this->nr_active_threads==0;});
	}
#include<iostream>
#include<vector>
#include<thread>
#include<queue>
#include<mutex>
#include<condition_variable>
#include<functional>


class ThreadPool{

    public:
        ThreadPool(const size_t thread_count): stop(false) { 
            
            for(size_t i = 0; i < thread_count; ++i){

                workers.emplace_back([this]() {

                    while(true){

                        std::function<void()> task;

                        {
                            std::unique_lock<std::mutex> lock(queue_mutex);

                            condition.wait(lock, [this](){
                                return (!tasks.empty() || stop);
                            });
                            
                            if(tasks.empty() && stop){
                                return;
                            }

                            task = std::move(tasks.front());
                            tasks.pop();
                        }
                        
                        task();
                        condition.notify_one();
                    }
                });

            }
        }

        void enqueue(std::function<void()> job){
            {
                std::unique_lock<std::mutex> lock(queue_mutex);

                tasks.push(job);
            }
        }

        ~ThreadPool(){
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                stop = true;
            }

            condition.notify_all();
            for(auto &worker: workers){
                worker.join();
            }
        }

    private:
        std::vector<std::thread>workers;
        std::queue<std::function<void()>>tasks;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop ;
};
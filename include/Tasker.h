#pragma once

struct Task {
    std::chrono::steady_clock::time_point scheduled_time;
    std::function<void()> func;

	Task() = default;
    Task(std::function<void()> f, const std::chrono::steady_clock::time_point time)
        : scheduled_time(time), func(std::move(f)) {}

    bool operator>(const Task& other) const {
        return scheduled_time > other.scheduled_time;
    }
};

class Tasker final : public clib_util::singleton::ISingleton<Tasker> {
public:
	static Tasker* GetSingleton();

    void Start(size_t num_threads = std::thread::hardware_concurrency());

    void Stop();

    bool IsRunning() const { return running_.load(std::memory_order_acquire); }
	bool HasTask() const {
		std::lock_guard lock(mutex_);
		return !task_queue_.empty();
	}


    template <typename Func, typename... Args>
    void PushTask(Func&& f, const int delay_ms, Args&&... args) {
        {
            auto bound_func = std::bind(std::forward<Func>(f), std::forward<Args>(args)...);
            const auto scheduled_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
            std::lock_guard lock(mutex_);
            task_queue_.emplace(std::move(bound_func), scheduled_time);
        }
        cv_.notify_one();
    }

private:
	Tasker();

	std::priority_queue<Task, std::vector<Task>, std::greater<>> task_queue_;
	mutable std::mutex mutex_;
	std::condition_variable cv_;
	std::atomic<bool> running_{ false };
	std::vector<std::thread> workers_;

	void WorkerLoop();
};

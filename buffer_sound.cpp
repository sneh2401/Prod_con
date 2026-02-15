#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <atomic>
#include <cmath>
#include <iomanip>

#define M_PI 3.14159265358979323846 

class AudioBuffer {
private:
    std::vector<float> buffer;      // Circular buffer for audio samples
    size_t capacity;
    size_t head = 0;                // Producer write position
    size_t tail = 0;                // Consumer read position
    size_t count = 0;               // Items currently in buffer
    std::mutex mtx;
    std::condition_variable not_empty;
    std::condition_variable not_full;
    std::atomic<long long> total_produced{0}, total_consumed{0};
    std::atomic<long long> max_latency_ns{0};
    std::atomic<bool> overflow{false};

public:
    AudioBuffer(size_t cap) : capacity(cap), buffer(cap) {}

    void produce(float sample) {
        std::unique_lock<std::mutex> lock(mtx);
        not_full.wait(lock, [this] { return count < capacity; });

        buffer[head] = sample;
        head = (head + 1) % capacity;
        ++count;
        ++total_produced;

        if (count == capacity) overflow = true;

        not_empty.notify_one();
    }  

    float consume() {
        std::unique_lock<std::mutex> lock(mtx);
        not_empty.wait(lock, [this] { return count > 0; });

        float sample = buffer[tail];
        tail = (tail + 1) % capacity;
        --count;
        ++total_consumed;

        not_full.notify_one();
        return sample;
    }

    void log_stats() const {
        std::cout << "Produced: " << total_produced << ", Consumed: " << total_consumed
                  << ", Max Latency: " << max_latency_ns << "ns, Overflow: " << (overflow ? "true" : "false") << std::endl;
    }
};

void producer(AudioBuffer& buf, double sample_rate, double duration_sec) {
    auto start = std::chrono::high_resolution_clock::now();
    double phase = 0.0;
    double freq = 440.0;  // A4 note

    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double>(now - start).count() > duration_sec) break;

        float sample = 0.5f * std::sin(phase);  // Generate sine wave sample
        buf.produce(sample);

        phase += 2 * M_PI * freq / sample_rate;  // Advance phase
    }
}

void consumer(AudioBuffer& buf, double duration_sec) {
    auto start = std::chrono::high_resolution_clock::now();
    long long samples_processed = 0;

    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double>(now - start).count() > duration_sec) break;

        float sample = buf.consume();
        // Simulate processing (apply gain, effects, etc.)
        samples_processed++;
    }
    std::cout << "Consumer processed " << samples_processed << " frames." << std::endl;
}

int main() {
    constexpr size_t BUFFER_CAPACITY = 1024;
    AudioBuffer buf(BUFFER_CAPACITY);

    constexpr double SAMPLE_RATE = 48000.0;
    constexpr double DURATION = 5.0;

    std::cout << "Real-Time Audio Buffer Simulation (Buffer size: " << BUFFER_CAPACITY << ")" << std::endl;

    std::thread prod(producer, std::ref(buf), SAMPLE_RATE, DURATION);
    std::thread cons(consumer, std::ref(buf), DURATION);

    std :: cout<<"helloe"<<endl;
    prod.join();
    cons.join();

    buf.log_stats();
    return 0;
}

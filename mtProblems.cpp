#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>
#include <numeric>
#include <condition_variable>
#include <unordered_map>

// Forward declaration of a method to ease testing.
void increment(int&);
long getLargePrime(int);

/** NOTE: You may add global variables as you see fit */
/**
 * The following printInOrder method is called from many threads, with
 * id (i.e., 0, 1, 2, ...) indicating the logical number of
 * thread. Implement the following method to print (to std::cout) the
 * supplied data in the order id value, as shown in the sample output.
 *
 * \param[in] The unique ID associated with the thread that is called
 * this method.
 *
 * \param[in] data The data to be printed by this thread.
 */
void printInOrder(const int id, std::string data) {
    static int turn = 0;
    static std::mutex turnMutex;
    static std::condition_variable locker;
    // Wait for my turn
    std::unique_lock<std::mutex> lock(turnMutex);    
    locker.wait(lock, [&]{ return turn == id; });
    std::cout << "id " << id << ": " << data << std::endl;
    // Next thread's turn
    turn++;
    locker.notify_all();
}
/**
 * Multithread each iteration of the following getSecret method
 *
 * \code
 *
 * long getSecret(char count) {
 *     long result = 1;
 *     for(int i = 0; (i < count); i++) {
 *         result *= getLargePrime(i);
 *     return result;
 * }
 * 
 * \endcode
 *
 * \param[in] count The number of iterations of the for-loop.
 */
long getSecret(char count) {
    long result = 1;
    for (int i = 0; (i < count); i++) {
        result *= getLargePrime(i);
    }
    return result;
}

/**
 * The focus of CSE major is to develop software
 * tools/libraries. Let’s work on such an example. A data science
 * software is running very slowly to find maximum value from billions
 * of numbers. Improve its performance by multithreading the following
 * getMax method to operate with k (k < 16) threads as a data parallel
 * program.
 *
 * \param[in] vec The vector to values from which the maximum value is
 * to be determined.
 *
 * \param[in] k The number of threads to be used by this method.
 *
 * \return The maximum value in vec.
 */
using IntVec = std::vector<int>;
int getMax(const IntVec& vec, int k) {
    // Helper method to compute max in a given range. This has been
    // adapted from the starter code.
    IntVec max(k);  // Create 'k' temporary max's from each thread.
    auto thrMax = [&](int i, int start, int end) {
        max[i] = *max_element(vec.begin() + start, vec.begin() + end); };
    // Evenly divide the nunber of elements between the threads
    const int count = (vec.size() + k - 1) / k;
    // Create the threads
    std::vector<std::thread> thr;
    for (int start = 0, i = 0; (i < k); i++, start += count) {
        auto end = std::min<int>(start + count, vec.size());
        thr.push_back(std::thread(thrMax, i, start, end));
    }
    // Wait for the threads to finish
    for (auto& t : thr) {
        t.join();
    }
    // Return the maximum from the intermediate max's
    return *max_element(max.begin(), max.end());
}

using UserMap = std::unordered_map<std::string, std::string>;
/**
 * This problem does not involve multithreading. Complete the
 * following method that: merges values (with a '-' between values)
 * with the same key in both maps (while ignoring keys that are not
 * present in both).
 *
 * \param[in] m1 The first map with keys and values to be merged.
 *
 * \param[in] m2 The second map with keys and values to be merged.
 *
 * \return The map with values associated with the same keys in m1 and
 * m2 (in that order) with a '-' between the values.
 */
UserMap merge(const UserMap& m1, const UserMap& m2) {
    UserMap m3;
    for (auto entry : m1) {
        auto key = entry.first;
        auto val = entry.second;
        if (m2.find(key) != m2.end()) {
            m3[key] = val + "-" + m2.at(key);
        }
    }
    return m3;
}

/**
 * The following method is called from a separate thread to produce a
 * series of numbers (i.e., 0, 1, 2, ..., n-1) in a shared queue for
 * another thread to process.  Complete this method to operate in
 * concert with the consumer method.
 *
 * NOTE: You must not modify the consumer method and the maximum queue
 * size is 5.
 *
 * \param[out] q The vector to which the must be added (to the end of
 * the queue).
 *
 * \param[out] qMutex A mutex that is shared between the producer and
 * consumer methods.
 *
 * \param[out] qCond A condition variable that is shared between the
 * producer and consumer methods.
 *
 * \param[in] n The maximum value up to (but not included) which a
 * series of numbers are to be generated by the producer.
 */
void producer(std::vector<int>& q, std::mutex& qMutex,
             std::condition_variable& qCondVar, int n) {
    for (int i = 0; (i < n); i++) {
        std::unique_lock<std::mutex> lock(qMutex);
        // Wait until queue is smaller than 5
        qCondVar.wait(lock, [&q]{ return q.size() < 5; });
        q.push_back(i);
        // If we forget to notify (i.e., wake-up the consumer) then
        // the program will hang.
        qCondVar.notify_one();
    }
}

/**
 * This method counts numbers that end with a given digit.  However,
 * it has not been correctly multithreaded.  Fix the issues with
 * this method so that it operates effectively.
 *
 * @param values The values to be distributed by this method.
 *
 * @param n The number of threads to be used.
 */
std::vector<int> countDigits(const std::vector<int> values, int n) {
    // Results to be returned by this method.  This vector has fixed
    // size of 10, because numbers can end with only one of the 10
    // digits.
    std::vector<int> counts(10);
    std::vector<std::mutex> mutexes(10);
    // Here is an inner-method that is run from separate threads.
    // Since it is a small method it has been placed here to
    // streamline working on this exam problem.
    auto thrMain = [&](int id) {
            for (size_t i = id; (i < values.size()); i += n) {
                const int digit = values[i] % 10;
                std::lock_guard<std::mutex> lock(mutexes[digit]);
                increment(counts[digit]);
            }
        };
    // Create the threads to run the loop below:
    std::vector<std::thread> thrList;
    for (int i = 0; (i < n); i++) {
        thrList.push_back(std::thread(thrMain, i));
    }
    // Wait for the threads to finish
    for (auto& t : thrList) {
        t.join();
    }
    // Return the results back to the caller
    return counts;
}

/**
 * The consumer method used for some basic testing of the operations
 * of the producer method.
 *
 * \param[out] q The vector to which the must be added (to the end of
 * the queue).
 *
 * \param[out] qMutex A mutex that is shared between the producer and
 * consumer methods.
 *
 * \param[out] qCond A condition variable that is shared between the
 * producer and consumer methods.
 *
 * \param[in] n The maximum value up to (but not included) which a
 * series of numbers are to be generated by the producer.
 *
 */
void consumer(std::vector<int>& q, std::mutex& qMutex,
             std::condition_variable& qCondVar, int n) {
    // Consume 'n' values that are going to be produced.
    for (int i = 0; (i < n); i++) {
        int value = -1;   // Changed in the CS below.
        
        {   // begin Critical Section (CS)
            std::unique_lock<std::mutex> lock(qMutex);
            // Wait until queue is not empty
            qCondVar.wait(lock, [&q]{ return !q.empty(); });
            // Extract value and process it outside the critical section.
            assert(q.size() <= 5);
            value = q.front();   // Get the value
            q.erase(q.begin());  // Erase the value
            // Let the producer know it can work more. If we forget to
            // notify (i.e., wake-up the producer) then
            // the program will hang.
            qCondVar.notify_one();
        }   // end Critical Section (CS)
        // Process the value
        assert(value == i);
    }
}

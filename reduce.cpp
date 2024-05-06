#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <unistd.h>
#include <iomanip>

constexpr int rows = 1000; /// < the number of rows in the work matrix
constexpr int cols = 100; /// < the number of cols in the work matrix

std::mutex stdout_lock; /// < for serializing access to stdout

std::mutex counter_lock; /// < for dynamic balancing only
volatile int counter = rows; /// < for dynamic balancing only

std::vector<int> tcount; /// < count of rows summed for each thread
std::vector<uint64_t> sum; /// < the calculated sum from each thread

int work[rows][cols]; /// < the matrix to be summed

int total_work;
uint64_t gross_sum;

/**
 * sum_static() uses static load balancing.
 *
 * This function sums up the rows in the 'work' matrix by "equally"
 * distributing the work to all specified cores. Less efficient.
 *
 * @param tid Integer representing the thread ID.
 * @param num_threads Default is 2, user specifies number of threads to use.
 *
 * @note counter_lock not needed for this function, nor the counter variable. 
 *       Total sum to be calculated is incremented using the work matrix and 
 *       each thread has its own value in the sum vector. This is then added 
 *       up at the end of main() in the variable gross_sum.
 *
 ********************************************************************************/

void sum_static(int tid, int num_threads)
{
    stdout_lock.lock();
    std::cout << "Thread " << tid << " starting" << std::endl;
    stdout_lock.unlock();

    bool done = false;

    int row = tid;

    while (!done)
    {
        if (row >= 1000)  // rows = 1000
        {
            done = true;
        }

        if (!done)
        {
            for (int i = 0; i < 100; i++)  // cols = 100
            {
                sum[tid] += work[row][i];
            }
            row += num_threads;
            ++tcount[tid];
        }
    }

    stdout_lock.lock();
    std::cout << "Thread " << tid << " ending tcount=" << tcount[tid] << " sum=" << sum[tid] << std::endl;
    stdout_lock.unlock();
}

/**
 * sum_dynamic() uses dynamic load balancing.
 *
 * This function sums up the rows in the 'work' matrix by 
 * distributing the workload to whichever core is available. More efficient.
 *
 * @param tid Integer representing the thread ID.
 *
 * @note counter_lock locks the critical section of the work process so that
 *       no other thread interferes with what is being calculated. 
 *       Total sum to be calculated is incremented using the work matrix and 
 *       each thread has its own value in the sum vector. This is then added 
 *       up at the end of main() in the variable gross_sum.
 *
 ********************************************************************************/

void sum_dynamic(int tid)
{
    stdout_lock.lock();
    std::cout << "Thread " << tid << " starting" << std::endl;
    stdout_lock.unlock();

    bool done = false;
    while (!done)
    {
        int count_copy;

        counter_lock.lock();
        {
        if (counter > 0)
        {
            --counter;
        }
        else
        {
            done = true;
        }
        count_copy = counter;  // For summing up the columns
        }
        counter_lock.unlock();

        if (!done)
        {
            (void)count_copy;

            ++tcount[tid];

            for (int i = 0; i < cols; i++)
            {
                sum[tid] += work[count_copy][i];
            }
        }
    }

    stdout_lock.lock();
    std::cout << "Thread " << tid << " ending tcount=" << tcount[tid] << " sum=" << sum[tid] << std::endl;
    stdout_lock.unlock();
}

/**
 * usage() describes how to use the program on the command line.
 *
 * @note For the -t option, you may use anywhere from 2 to maxThreads,
 *       which is however many threads your system supports.
 *
 ********************************************************************************/

static void usage()
{
	std::cerr << "Usage: rv32i [-d] [-t num]" << std::endl;
	std::cerr << "    -d Use dynamic load-balancing. (Default: static)" << std::endl;
    std::cerr << "    -t Specifies the number of threads to use. (Default: 2)" << std::endl;
	exit(1);
}

/**
 * main() initializes thread pointers, joins them at the end, and sums the totals.
 *
 * @param argc How many arguments were entered on the command line. (-d, -t)
 * @param argv Array of pointers of arrays of char objects.
 *
 * @note Displays how many threads are supported and error checks out of bound values.
 *
 ********************************************************************************/

int main(int argc, char **argv)
{
    int opt;
    bool dFlag = false;
    uint32_t limiter = 2;
    unsigned int maxThreads = std::thread::hardware_concurrency();

    srand(0x1234);

    while ((opt = getopt(argc, argv, "dt:")) != -1)
	{
		switch (opt)
		{
		case 'd':
		{
            dFlag = true;
		}
			break;
		case 't':
		{
			limiter = atoi(optarg);

            if (limiter < 2)   // I would put 1, but what would the purpose be 
            {                  // if the point is to use multithreading?
                limiter = 2;
            }
            else if (limiter > maxThreads)
            {
                limiter = maxThreads;
            }
		}
			break;
		default:
			usage();
		}
	}

    tcount.resize(limiter, 0);
    sum.resize(limiter, 0);

    std::cout << std::thread::hardware_concurrency() << " concurrent threads supported." << std::endl;

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            work[i][j] = rand();
        }    
    }

    std::vector<std::thread*> threads;

    if (dFlag == true) // Dynamic load balancing
    {
        for (uint32_t i = 0; i < limiter; ++i)
        {
            threads.push_back(new std::thread(sum_dynamic, i));
        }
    }
    else  // Static load balancing
    {
        for (uint32_t i = 0; i < limiter; ++i)
        {
            threads.push_back(new std::thread(sum_static, i, limiter));
        }
    }

    for (uint32_t i = 0; i < limiter; ++i)  // Adding totals 
    {
        threads.at(i)->join();
        delete threads.at(i);
        total_work += tcount.at(i);
        gross_sum += sum.at(i);
    }

    std::cout << "main() exiting, total_work=" << total_work 
              << " gross_sum=" << gross_sum << std::endl;

    return 0;
}
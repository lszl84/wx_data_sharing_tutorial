#include <algorithm>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

std::vector<std::string> sharedData = {"abc", "def", "ghi"};
std::mutex m;

void printingThreadMethod()
{
    while (true)
    {
        {
            std::lock_guard g(m);
            for (const auto &item : sharedData)
            {
                std::cout << item << " ";
            }
        }

        std::cout << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void shufflingThreadMethod()
{
    std::random_device rd;
    std::mt19937 gen(rd());

    while (true)
    {
        {
            std::lock_guard g(m);
            std::shuffle(sharedData.begin(), sharedData.end(), gen);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int main()
{
    std::thread printingThread{printingThreadMethod};
    std::thread shufflingThread{shufflingThreadMethod};

    printingThread.join();
    shufflingThread.join();
}
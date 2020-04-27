#include "src/RingBuffer.hpp"
#include <fstream>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <iostream>

class RingBufferMutexImpl : public RingBufferMutex
{
public:
	RingBufferMutexImpl() {}
	virtual ~RingBufferMutexImpl() {}
	virtual void lock() 
	{
		m_mutex.lock();
	};
	virtual void unlock() 
	{
		m_mutex.unlock();
	};

private:
	boost::mutex m_mutex;
};

RingBuffer<RingBufferMutexImpl> ringBuffer;

uint64_t writeTotalBytes = 0;
uint64_t readTotalBytes = 0;
bool isEnd = false;
void writeThread()
{
	FILE* file = fopen("F:/05_TEST_TJ44071_OP_SCSJ_U20200402_064651_123_101_00011.dat", "rb");
	char* buffer = new char[512];
	if (file)
	{
		time_t begin_time = 0;
		time_t total_time = 0;
		while ( !feof(file) )
		{
			size_t realReadSize = fread(buffer, 1,512, file);
			if (realReadSize > 0)
			{
				uint64_t size = 0;
				while ((size = ringBuffer.write(buffer, realReadSize)) == 0)
				{
					//std::cout << "write buffer not enough " << std::endl;
					Sleep(1);
				}
				writeTotalBytes += size;
				if (begin_time == 0)
					begin_time = time(0);
				else
				{
					time_t end_time = time(0);
					if (end_time - begin_time >= 1)
					{
						total_time += end_time - begin_time;
						begin_time = end_time;
						std::cout << "average write rate:" << writeTotalBytes / total_time / 1024 / 1024 << " MB/S" << std::endl;
					}
				}
			}
		}
		std::cout << "writeThread finish bytes = " << writeTotalBytes << std::endl;
	}
	isEnd = true;
}

bool readStop = false;
void readThread()
{
	std::fstream file;
	file.open("D:/out.dat", std::ios::out | std::ios::binary | std::ios::trunc);
	uint64_t count = 0;

	time_t begin_time = 0;
	time_t total_time = 0;
	if (file.is_open())
	{
		char *buffer = new char[500];
		uint64_t size = 0;
		for (;;)
		{
			if(readStop)
				break;
			size = ringBuffer.read(buffer, 212);
			readTotalBytes += size;
			if (size > 0)
			{
				file.write(buffer, size);
				count = 0;
			}
			else
			{
				//std::cout << "read buffer no data " << std::endl;
				if(isEnd)
					break;
				Sleep(1);
			}
			if (begin_time == 0)
				begin_time = time(0);
			else
			{
				time_t end_time = time(0);
				if (end_time - begin_time >= 1)
				{
					total_time += end_time - begin_time;
					begin_time = end_time;
					std::cout << "average read rate:" << writeTotalBytes / total_time / 1024 / 1024 << " MB/S" << std::endl;
				}
			}
		}
		file.close();
	}
	std::cout << "read bytes = " << readTotalBytes << std::endl;
}

int main()
{
	ringBuffer.initialize(1024 * 1024 * 100);
	boost::thread writeThread(writeThread);
	boost::thread readThread(readThread);

	writeThread.join();
	std::cout << "writeThread end" << std::endl;
	int n = 0;
	std::cin >> n;
	readStop = true;

	readThread.join();
	std::cout << "readThread end" << std::endl;

	std::cin >> n;
	std::cout << "ok" << std::endl;
	return 0;
}
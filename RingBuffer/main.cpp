#include "src/RingBuffer.h"
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
void writeThread()
{
	FILE* file = fopen("test.dat", "rb");
	char buffer[512];
	if (file)
	{
		while ( !feof(file) )
		{
			size_t realReadSize = fread(buffer, 1,512, file);
			if (realReadSize > 0)
			{
				uint64_t size = 0;
				while ((size = ringBuffer.write(buffer, realReadSize)) == 0)
				{
					std::cout << "write buffer not enough " << std::endl;
					Sleep(1);
				}
				writeTotalBytes += size;
			}
		}
		std::cout << "writeThread finish bytes = " << writeTotalBytes << std::endl;
	}
}

bool readStop = false;
void readThread()
{
	std::fstream file;
	file.open("out.dat", std::ios::out | std::ios::binary | std::ios::trunc);
	uint64_t count = 0;
	if (file.is_open())
	{
		char buffer[512];
		uint64_t size = 0;
		for (;;)
		{
			if(readStop)
				break;
			size = ringBuffer.read(buffer, 500);
			readTotalBytes += size;
			if (size > 0)
			{
				file.write(buffer, size);
				count = 0;
			}
			else
			{
				Sleep(1);
			}
		}
		file.close();
	}
	std::cout << "read bytes = " << readTotalBytes << std::endl;
}

int main()
{
	ringBuffer.initialize(1024 * 1024 * 50);
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
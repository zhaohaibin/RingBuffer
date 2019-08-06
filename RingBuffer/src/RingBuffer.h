#include <stdint.h>
#ifndef RING_BUFFER_H
#define RING_BUFFER_H

class RingBufferMutex
{
public:
	RingBufferMutex() {}
	virtual ~RingBufferMutex() {}

	virtual void lock() {};
	virtual void unlock() {};
};

template<typename T>
class RingBuffer
{
	class Locker
	{
	public:
		Locker(T& mutex)
			:m_mutex(mutex)
		{
			m_mutex.lock();
		}
		~Locker()
		{
			m_mutex.unlock();
		}
		T& m_mutex;
	};
public:
	RingBuffer()
		: m_wPos(-1)
		, m_rPos(-1)
		, m_length(0)
		, m_pBuffer(nullptr)
	{

	}

	~RingBuffer()
	{
		release();
	}

	void release()
	{
		if (m_pBuffer != nullptr)
		{
			delete[]m_pBuffer;
			m_pBuffer = nullptr;
		}
		m_wPos = -1;
		m_rPos = -1;
		m_length = 0;
	}

	bool initialize(int64_t size)
	{
		if (size <= 0) return false;

		m_pBuffer = new char[size];
		m_wPos = 0;
		m_length = size;
		return true;
	}

	//���ؿ���д����ڴ��С
	int64_t getLengthForWrite()
	{
		Locker locker(m_mutex);
		return doGetLengthForWrite();
	}

	//���ؿɶ�ȡ���ڴ��С
	int64_t getLengthForRead()
	{
		Locker locker(m_mutex);
		return doGetLengthForRead();
	}

	//д������
	int64_t write(char* pData, int64_t length)
	{
		Locker locker(m_mutex);
		int64_t lengthForWrite = doGetLengthForWrite();
		if (lengthForWrite < length)
			return 0;
		if (lengthForWrite > 0)
		{
			int64_t realWriteLength = (lengthForWrite > length) ? length : lengthForWrite;
			int64_t beginPos = m_wPos;
			int64_t endPos = (beginPos + realWriteLength) % m_length;
			if ((endPos-1) >= beginPos)
			{
				memcpy(m_pBuffer + beginPos, pData, realWriteLength);
			}
			else
			{
				int64_t len1 = m_length - beginPos;
				memcpy(m_pBuffer + beginPos, pData, len1);
				int64_t len2 = realWriteLength - len1;
				memcpy(m_pBuffer, pData + len1, len2);
			}

			//�жϻ������Ƿ����������������дλ����Ϊ-1
			if (endPos == m_rPos || endPos == beginPos)
				endPos = -1;

			//��������д������֮ǰ�������ǿգ���ʱ��Ҫ����ȡλ�����õ���ȷ��λ����
			if (m_rPos == -1)
				m_rPos = m_wPos;

			m_wPos = endPos;
			return realWriteLength;
		}
		return 0;
	}

	//��ȡ����
	int64_t read(char* pData, int64_t length)
	{
		Locker locker(m_mutex);
		int64_t lengthForRead = doGetLengthForRead();
		if (lengthForRead > 0)
		{
			int64_t realReadLength = (lengthForRead > length) ? length : lengthForRead;
			int64_t beginPos = m_rPos;
			int64_t endPos = (beginPos + realReadLength) % m_length;
			if ((endPos - 1) >= beginPos)
			{
				memcpy(pData, m_pBuffer + beginPos, realReadLength);
			}
			else
			{
				int64_t len1 = m_length - beginPos;
				memcpy(pData, m_pBuffer + beginPos, len1);
				int64_t len2 = realReadLength - len1;
				memcpy(pData + len1, m_pBuffer, len2);
			}

			//�ж������Ƿ񱻶���
			if (endPos == m_wPos || endPos == beginPos)
				endPos = -1;

			//�������ζ�ȡ֮ǰ����������������Ҫ����дλ��
			if (m_wPos == -1)
				m_wPos = m_rPos;
			m_rPos = endPos;
			return realReadLength;
		}
		return 0;
	}

private:
	int64_t doGetLengthForWrite()
	{
		if (m_wPos == -1 && m_rPos == -1)
			return -1;

		/*
		case 1
		������Ϊ��,��ʱ��ȡλ��Ϊ-1
		*/
		if (m_rPos == -1)
			return m_length;

		/*
		case 2
		��������,��ʱдλ��Ϊ-1
		*/
		if (m_wPos == -1)
			return 0;

		/*
		case 3
		д��λ�ô��ڶ�ȡλ��
		*/
		if (m_wPos > m_rPos)
		{
			return m_length - (m_wPos - m_rPos);
		}

		/*
		case 4
		д��λ��С�ڶ�ȡλ��
		*/
		return m_rPos - m_wPos;
	}

	int64_t doGetLengthForRead()
	{
		int64_t lengthForWrite = doGetLengthForWrite();
		if (lengthForWrite >= 0)
			return m_length - lengthForWrite;
		return -1;
	}
private:
	int64_t m_wPos;
	int64_t m_rPos;
	int64_t m_length;
	char* m_pBuffer;
	T m_mutex;
};
#endif //RING_BUFFER_H
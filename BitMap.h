#ifndef BITMAP_H
#define BITMAP_H
#include <malloc.h>
#include <assert.h>

const int MAXNUM = 4096;

class BitMap
{
public:
	BitMap()
	{

	}
	BitMap(int length, char *data)
	{
		if (data == NULL)
		{
			return;
		}
		//assert(data);
		length_ = length;
		bitpos_ = data;
	}
	~BitMap()
	{
		if (bitpos_)
		{
			delete bitpos_;
		}
	}

	void Set(int x)
	{
		int index = x >> 3;
		int pos = x % 8;
		*(bitpos_ + index) |= (1 << pos);
	}
	void Reset(int x)
	{
		int index = x >> 3;
		int pos = x % 8;
		*(bitpos_ + index) &= (~(1 << pos));
	}
	int Test(int x)
	{
		int index = x >> 3;
		int pos = x % 8;
		if ((*(bitpos_ + index)) & (1 << pos))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	// 返回从第n个开始第一个值为val的位置，没有则返回-1
	int FirstBit(int n, int val)
	{
		int index = n >> 3;
		int pos = n % 8;
		int num = 0;
		while (((index << 3) + pos) <= length_*8)
		{
			num = ((index << 3) + pos);
			if (((*(bitpos_ + index) & (1<<pos)) >> pos) == val)
			{
				return num;
			}
			if (pos == 7)
			{
				pos = 0;
				index++;
			}
			else
			{
				pos++;
			}
		}
		return -1;
	}
	// 判断是否有值为0的位
	bool hasZero()
	{
		int index = 0;
		int pos = 0;
		while (((index << 3) + pos) <= length_ * 8)
		{
			if (((*(bitpos_ + index) & (1 << pos)) >> pos) == 0)
			{
				return true;
			}
			if (pos == 7)
			{
				pos = 0;
				index++;
			}
			else
			{
				pos++;
			}
		}
		return false;
	}

private:
	int length_;  // 位图占的字节大小
	char* bitpos_;  // 位图起始位置
};

#endif

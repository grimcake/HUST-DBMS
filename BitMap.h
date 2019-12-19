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
	// ���شӵ�n����ʼ��һ��ֵΪval��λ�ã�û���򷵻�-1
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
	// �ж��Ƿ���ֵΪ0��λ
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
	int length_;  // λͼռ���ֽڴ�С
	char* bitpos_;  // λͼ��ʼλ��
};

#endif

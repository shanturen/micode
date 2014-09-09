#include "system.h"
#include "buffer.h"
inline static int round_size(int size)
{
	return size;
}

buffer::buffer(int size)
{
	if (size == 0) {
		_compacity = 0;
		_used_size = 0;
		_begin = 0;
	}
	_compacity = round_size(size);
	_data = new unsigned char[_compacity];
	_begin = _used_size = 0;
}

buffer::buffer(const buffer &rhs)
{
	_data = 0;
	_used_size = rhs._used_size;
	if (rhs._used_size > 0) {
		_data = new unsigned char [rhs._used_size];
		memcpy(_data, rhs._data + rhs._begin, rhs._used_size);
	}
	_begin = 0;
	_compacity = _used_size;
}

buffer &buffer::operator= (const buffer &rhs)
{
	_data = 0;
	_used_size = rhs._used_size;
	if (rhs._used_size > 0) {
		_data = new unsigned char [rhs._used_size];
		memcpy(_data, rhs._data + rhs._begin, rhs._used_size);
	}
	_begin = 0;
	_compacity = _used_size;
	return *this;
}

int buffer::append_data(const void *data, int length)
{
	if (ensure_compacity(_used_size + length) < 0)
		return -1; // system lack of memory
	memcpy(_data + _begin + _used_size, data, length);
	_used_size += length;
	return 0;
}

int buffer::drain_data(void *data, int length)
{
	if (_used_size < length)
		return -1;
	memcpy(data, _data + _begin, length);
	_used_size -= length;
	_begin += length;
	if (_used_size == 0)
		_begin = 0;
	return 0;
}

int buffer::ensure_compacity(int size)
{
	if (_compacity - _begin - _used_size > size)
		return 0;
	if (_compacity - _used_size > size) {
		for (int i = 0; i != _used_size; i++)
			_data[i] = _data[_begin + i];
		_begin = 0;
		return 0;
	}

	int compacity = round_size(_used_size + size);
	unsigned char *data = new unsigned char [compacity];
	if (!data)
		return -1;
	memcpy(data, _data + _begin, _used_size);
	delete [] _data;
	_data = data;
	_compacity = compacity;
	_begin = 0;
	return 0;
}


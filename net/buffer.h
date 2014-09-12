#pragma once
#include <string>
// what i really want ?
// what should a smart buffer do?
class buffer
{
	unsigned char *_data;
	int _compacity;
	int _used_size;
	int _begin;
public:
	buffer(int size = 0);
	buffer(const buffer &rhs);
	buffer &operator= (const buffer &rhs);
	~buffer() { if (_data) delete [] _data; }
	int append_data(const void *data, int length);
	int drain_data(void *data, int length);
	void *get_data_buf() const { return _data + _begin; } 
	int get_size() const { return _used_size; }

	// 妈的，不满意
	int ensure_compacity(int size);
	void drain_all_data() { _begin = _used_size = 0; }
	void set_used_size(int size) { _used_size += size; }

	int append_string(const std::string &s);
};


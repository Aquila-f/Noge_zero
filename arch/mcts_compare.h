#pragma once
#include "struct.h"


class compare{
public:
	virtual bool compare_result(const int& bas){return true;}
};

class time_compare : public compare{
public:
	time_compare(const int& limit_range) : compare(), time_limit_(limit_range){
		time_limit_ = millisec() + limit_range;
	}
public:
	virtual bool compare_result(const int& bas){
		if(bas%500 != 0) return false;
		if(millisec() > time_limit_) return true;
		return false;
	}
	uint64_t millisec() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}
private:
	uint64_t time_limit_;
};

class count_compare : public compare{
public:
	count_compare(const int& count_limit) : compare(), count_limit_(count_limit){}
public:
	virtual bool compare_result(const int& count){
		if(count == count_limit_) return true;
		return false;
	}
private:
	uint64_t count_limit_;
};

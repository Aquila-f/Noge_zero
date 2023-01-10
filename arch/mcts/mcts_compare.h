#pragma once
#include "struct.h"


class compare{
public:
	compare(){}
public:
	virtual bool compare_result(const int& bas){return true;}
	virtual void new_game(){}
};

class time_compare : public compare{
public:
	time_compare(const uint64_t& limit_range, const int& game_step) : compare(), game_step(0), time_limit_(limit_range){
		// time_limit_ = millisec() + setup_time(limit_range, game_step);
	}
public:
	virtual bool compare_result(const int& simulation_counts){
		if(simulation_counts%500 != 0) return false;
		if(millisec() > time_limit_){
			game_step++;
			return true;
		}
		return false;
	}
	int setup_time(const int& limit_range, const int& game_step){
		return (2-0.05*game_step)*limit_range;
	}

	uint64_t millisec() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}
private:
	int game_step;
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
	int count_limit_;
};

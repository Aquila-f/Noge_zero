#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
// #include <ctime>
#include <unordered_map>
// #include <stack>
// #include <type_traits>
// #include <algorithm>
// #include <ctime>
#include <chrono>


#include "board.h"
#include "action.h"
#include "agent.h"
#include <thread>



struct node{
    int move_position;
    double win_count;
    double visit_count;
    int available_node_count;
    std::vector<node*> level_vector;
    std::vector<int> empty_vector;
    node() : win_count(0), visit_count(0), available_node_count(-1){}
    node(std::vector<int> inp) : win_count(0), visit_count(0), available_node_count(-1), empty_vector(inp){}
    node(const board& b) : win_count(0), visit_count(0), available_node_count(-1){
        for(int i=0;i<81;i++){
			if(b[i/9][i%9] == 0) empty_vector.push_back(i);
		}
    }
    ~node(){for(auto n : level_vector) delete n;}

    void level_vec_init(const board& state){
        board after;
        for(int i=0;i<empty_vector.size();i++){
            after = state;
            if(after.place(empty_vector[i]) == board::legal){
                node* tmp = new node(empty_vector);
                tmp->empty_vector.erase(tmp->empty_vector.begin()+i);
                tmp->move_position = empty_vector[i];
                level_vector.push_back(tmp);
            }
        }
        available_node_count = level_vector.size();
    }
};

class timer{
public:
	timer(){}
	~timer(){}
protected:
	time_t millisec() {
		auto now = std::chrono::system_clock::now().time_since_epoch();
		return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
	}
};
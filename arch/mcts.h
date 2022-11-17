#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <unordered_map>
#include <stack>
#include <type_traits>
#include <algorithm>
#include <ctime>

#include "board.h"
#include "action.h"
#include "agent.h"

#include <unistd.h>
#include <thread>
#include <fstream>



struct node{
    int move_position;
    double win_count;
    double visit_count;
    int available_node_count;
    std::vector<node*> level_vector;
    std::vector<int> random_vector;
    std::vector<int> empty_vector;
    node() : win_count(0), visit_count(0), available_node_count(-1){}
    node(std::vector<int> inp) : win_count(0), visit_count(0), available_node_count(-1), empty_vector(inp){}
    node(board& b) : win_count(0), visit_count(0), available_node_count(-1){
        for(int i=0;i<81;i++){
			if(b[i/9][i%9] == 0) empty_vector.push_back(i);
		}
    }

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

    // void node_level_vector_update(std::vector<int> previous_node_vector){
    //     for(auto )
    // }
};



// class mcts_tree : public basic_agent_func{
// public:
//     mcts_tree(const std::string& args = "") : basic_agent_func(args){
//         simulation_count_ = 100;
//     }
    
//     node* simulate_result(){
//         node* root = new node();

//         for(int i=0;i<simulation_count_;i++){
//             // Selection()
//         }
        
//         return root;
//     }
    
//     node* Selection(node* root, board& state){
//         update_node_vector.push_back(root);
//         // node init
//         board after = state;
//         if(root->available_node_count == -1){
//             int ava_node_count = 0;
//             node* tmpnode = new node[81];
//             release_node_vector.push_back(tmpnode);
//             for(int i=0;i<81;i++){
//                 root->level_vector.push_back(&tmpnode[i]);
//                 after = state;
//                 if(after.place(i) == board::legal){
//                     tmpnode[i].move_position = i;
//                     root->random_vector.push_back(i);
//                     ava_node_count++;
//                 }
//             }
//             root->available_node_count = ava_node_count;
//             if(ava_node_count==0) return;
//             std::shuffle(root->random_vector.begin(), root->random_vector.end(), engine);
//             // return root;
//         }
//         if(root->random_vector.size() == 0) return;
        
//     }

//     // node* Selection_find_leaf(node* root, board& state){
//     //     if(root->available_node_count == -1)
//     // }

//     // node* Selection_leaf_init(node* root, board& state){
//     //     if(root->available_node_count == -1)
//     // }

//     void Expansion(){
        
//     }

//     void Rollout(){
        
//     }

//     void Backpropagation(){
        
//     }
    
    
// private:
//     int thread_id;
// 	std::vector<action::place> space_;
// 	std::vector<action::place> space_a_;

// 	// std::vector<int> play_sequence;
// 	std::vector<node*> update_node_vector;
// 	std::vector<node*> release_node_vector;
// 	bool self_simulate_win;
// 	std::string enemy_state_playmode_;
	
// 	double c_;
// 	double simulation_count_;
// 	double simulation_time_;
// };
/**
 * Framework for NoGo and similar games (C++ 11)
 * agent.h: Define the behavior of variants of the player
 *
 * Author: Theory of Computer Games
 *         Computer Games and Intelligence (CGI) Lab, NYCU, Taiwan
 *         https://cgilab.nctu.edu.tw/
 */

#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include <fstream>
#include "board.h"
#include "action.h"
#include "mcts.h"
// #include "players.h"

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b) { return action(); }
	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;

protected:
	std::default_random_engine engine;
};


/**
 * base agent for agents with randomness
 */
class basic_agent_func : public agent {
public:
	basic_agent_func(const std::string& args = "") : agent(args),
	space(board::size_x * board::size_y), who(board::empty) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
		if (name().find_first_of("[]():; ") != std::string::npos)
			throw std::invalid_argument("invalid name: " + name());
		if (role() == "black") who = board::black;
		if (role() == "white") who = board::white;
		if (who == board::empty)
			throw std::invalid_argument("invalid role: " + role());
		for (size_t i = 0; i < space.size(); i++)
			space[i] = action::place(i, who);
	}
	virtual ~basic_agent_func() {}

protected:
	std::default_random_engine engine;
	std::vector<action::place> space;
	board::piece_type who;
};

/**
 * random player for both side
 * put a legal piece randomly
 */

class random_action : public basic_agent_func {
public:
	random_action(const std::string& args = "") : basic_agent_func(args){}
	virtual action take_action(const board& state) {
		std::shuffle(space.begin(), space.end(), engine);
		for (const action::place& move : space) {
			board after = state;
			if (move.apply(after) == board::legal)
				return move;
		}
		return action();
	}
};




class mcts_tree : public basic_agent_func{
public:
    mcts_tree(const std::string& args = "") : basic_agent_func(args){
        simulation_count_ = 1;
    }

    	int check_board_info(const int p, const board& b){
		return b[p/9][p%9];
	}

    action simulate_result(board& state){
        node* root = new node(state);
		board after;
		
		int sim_tim = 100;
		for(int i=0;i<sim_tim;i++){
			after = state;
			node* ii = Selection_Expansion(root, after);
			board::piece_type losser = Rollout(ii, after);
			Backpropagation(losser);
		}

		if(false){
			std::cout << root->empty_vector[0] << std::endl;
			std::cout << state;
			std::cout << state.info().who_take_turns << std::endl;
			print_node(root);
			for(auto n : root->level_vector){
				print_node(n); 
			}
		}


		if(root->level_vector.size() == 0) return action();
		int max_visit_n=-1;
		int max_visit_pos=-1;
		for(auto subn : root->level_vector){
			if(subn->visit_count > max_visit_n){
				max_visit_n = subn->visit_count;
				max_visit_pos = subn->move_position;
			}
		}
        return action::place(max_visit_pos, who);
    }

    node* Selection_Expansion(node* root, board& state, int state_flag=1){
		update_node_vector.push_back(root);
		if(root->available_node_count == -1){
			root->level_vec_init(state);
			std::shuffle(root->level_vector.begin(), root->level_vector.end(), engine);
		}
		if(root->available_node_count==0) return root;
		double max_uct = -2;
		double uct;
		int max_uct_idx = -1;
		for(int i=0;i<root->level_vector.size();i++){
			if(root->level_vector[i]->visit_count == 0){
				state.place(root->level_vector[i]->move_position);
				update_node_vector.push_back(root->level_vector[i]);
				return root->level_vector[i];
			}else{
				uct = root->level_vector[i]->win_count/root->level_vector[i]->visit_count+c_*sqrt(log(root->visit_count)/root->level_vector[i]->visit_count);
				if(max_uct < uct){
					max_uct = uct;
					max_uct_idx = i;
				}
			}
		}
		state.place(root->level_vector[max_uct_idx]->move_position);
		return Selection_Expansion(root->level_vector[max_uct_idx], state, state_flag*-1);
    }	
	
	board::piece_type Rollout(node* root, board&state){
		std::vector<int> tmp = root->empty_vector;
		// std::vector<int> tmp2 = root->empty_vector;
		std::shuffle(tmp.begin(), tmp.end(), engine);
		// std::shuffle(tmp2.begin(), tmp2.end(), engine);
		int move_count = tmp.size()-1;
		for(int i=0;i<tmp.size();i++){
			if(state.place(tmp[i]) == board::legal){
				for(;move_count>=0;move_count--){
					if(state.place(tmp[move_count]) == board::legal){						
						break;
					}
				}
				if(move_count == -1) break;
			}
		}
        return state.info().who_take_turns;
    }

	void Backpropagation(board::piece_type losser){
		int win_value = who == losser ? 0 : 1;
        for(auto update_node : update_node_vector){
			update_node->visit_count += 1;
			update_node->win_count += win_value;
		}
		update_node_vector.clear();
    }

	void print_node(node* n){
		std::cout << "visit_count " << n->visit_count << ", ";
		std::cout << "win_count " << n->win_count << ", ";
		std::cout << "ava_n_count " << n->available_node_count << ", ";
		std::cout << "move_position" << n->move_position << ", ";
		std::cout << "empty_vector" << n->empty_vector.size() << ", ";
		std::cout << std::endl;
	}

    
    
    
private:
    int thread_id;
	std::vector<action::place> space_;
	std::vector<action::place> space_a_;

	// std::vector<int> play_sequence;
	std::vector<node*> update_node_vector;
	std::vector<node*> release_node_vector;
	bool self_simulate_win;
	std::string enemy_state_playmode_;
	
	double c_=0.3;
	double simulation_count_;
	double simulation_time_;
};

class mcts_action : public mcts_tree {
public:
	mcts_action(const std::string& args = "") : mcts_tree(args){}
	virtual void close_episode(const std::string& flag = "") {time_management = 1;}
	virtual action take_action(const board& state) {
		board after = state;
		// std::cout << after;
		return simulate_result(after);
	}
private:
	double time_management;
};



class player : public basic_agent_func {
public:
	player(const std::string& args = "") : basic_agent_func(args) {
		std::cout << args << std::endl;
		if (name() == "mcts"){
			std::cout << "mcts" << std::endl;
			policy = new mcts_action(args);
		}else{
			std::cout << "random" << std::endl;
			policy = new random_action(args);
			// ((random_action*)policy)->a();
		}
	}

	virtual action take_action(const board& state) {
		return policy->take_action(state);
	}

private:
	agent* policy;
};



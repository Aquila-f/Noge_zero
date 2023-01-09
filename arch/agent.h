/**
 * Framework for NoGo and similar games (C++ 11)
 * agent.h: Define the behavior of variants of the player
 *
 * Author: Theory of Computer Games
 *         Computer Games and Intelligence (CGI) Lab, NYCU, Taiwan
 *         https://cgilab.nctu.edu.tw/
 */

#pragma once
#include "mcts/mcts_management.h"
#include <unordered_map>
#include <map>
#include <thread>
// #include "struct.h"


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
	virtual std::string search() const { return property("search"); }
	virtual float mcts_c() const { return stof(property("c")); }
	virtual int total_simulation_count() const { return stoi(property("N")); }
	virtual int total_simulation_time() const { return stoi(property("T"))/36; }
	virtual int thread_num() const { return stoi(property("thread")); }

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
	std::vector<action::place> space;
	board::piece_type who;
	int play_step;
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

class mcts_action : public basic_agent_func{
public:
	mcts_action(const std::string& args = ""): basic_agent_func(args), args_(args), thread_num_(1){
		std::cout << "0" << std::endl;
		if (thread_num() != 1) thread_num_ = thread_num();
	}
	virtual void close_episode(const std::string& flag = "") {game_step=0;}
	
	std::unordered_map<int, int> thread_simulate_result(const board& state){
		std::vector<std::thread> threads;
		std::vector<mcts_management> thread_mcts;
		std::vector<node*> thread_mcts_root;
		
		for(int i=0;i<thread_num_;i++){
			node* thread_root = new node(state);
			thread_mcts_root.push_back(thread_root);
			mcts_management tmp_mcts(who, game_step, total_simulation_count(), total_simulation_time(), mcts_c());
			thread_mcts.push_back(std::move(tmp_mcts));
			std::thread th(thread_mcts[i], state, thread_root);
			threads.push_back(std::move(th));
		}

		for(int i=0;i<thread_num_;i++){
			threads[i].join();
		}
		
		std::unordered_map<int, int> node_table;

		for(auto ro : thread_mcts_root){
			for(auto n : ro->level_vector){
				node_table[n->move_position] += n->visit_count;
				delete n;
			}
		}
		// std::cout << state;

		return node_table;
	}

	void print_node(node* n){
		std::cout << "visit_count " << n->visit_count << ", ";
		std::cout << "win_count " << n->win_count << ", ";
		std::cout << "ava_n_count " << n->available_node_count << ", ";
		std::cout << "move_position" << n->move_position << ", ";
		std::cout << "empty_vector" << n->empty_vector.size() << ", ";
		std::cout << std::endl;
	}

	virtual action take_action(const board& state) {
		std::unordered_map<int, int> mcts_root = thread_simulate_result(state);
		if(mcts_root.size() == 0) return action();
		int max_visit_pos = -1;
		int max_visit_time = -1;

		for(auto subn : mcts_root){
			if(subn.second > max_visit_time){
				max_visit_pos = subn.first;
				max_visit_time = subn.second;
			}
		}
		game_step++;
        return action::place(max_visit_pos, who);
	}

private:
	std::string args_;
	int game_step;
	int thread_num_;
};



class player : public basic_agent_func {
public:
	player(const std::string& args = "") : basic_agent_func(args), args_(args) {
		// std::cout << args << std::endl;
		if (search() == "mcts"){
			std::cout << "mcts" << std::endl;
			policy = new mcts_action(args);
		}else{
			std::cout << "random" << std::endl;
			policy = new random_action(args);
		}
	}
	virtual action take_action(const board& state) {
		return policy->take_action(state);
	}
	virtual void close_episode(const std::string& flag = "") {
		policy->close_episode();
	}

private:
	std::string args_;
	agent* policy;
};



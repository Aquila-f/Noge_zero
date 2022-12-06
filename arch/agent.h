/**
 * Framework for NoGo and similar games (C++ 11)
 * agent.h: Define the behavior of variants of the player
 *
 * Author: Theory of Computer Games
 *         Computer Games and Intelligence (CGI) Lab, NYCU, Taiwan
 *         https://cgilab.nctu.edu.tw/
 */

#pragma once
#include "struct.h"


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
	virtual std::string mcts_c() const { return property("c"); }
	virtual std::string total_simulation_count() const { return property("N"); }
	virtual std::string total_simulation_time() const { return property("T"); }
	virtual std::string thread_num() const { return property("thread"); }

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
	
	// time_t now_t(){
	// 	return std::chrono::steady_clock::now();
	// }
	virtual bool mcts_break(){ return true; }
	virtual ~basic_agent_func() {}
protected:
	uint64_t millisec() {
		auto now = std::chrono::system_clock::now().time_since_epoch();
		return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
		// return std::chrono::duration_cast<std::chrono::milliseconds>(now);
	}
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

class mcts_tree: public basic_agent_func{
public:
	mcts_tree(const std::string& args = "") : basic_agent_func(args),
		c_(0.3){
		if (mcts_c() != "0.3") c_ = stof(mcts_c());
    }


	void MCTS_simulate(node* root, board& after){
		node* leaf = Selection_Expansion(root, after);
		board::piece_type losser = Rollout(leaf, after);
		Backpropagation(losser);
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
				uct = get_Q(root, i);
				if(max_uct < uct){
					max_uct = uct;
					max_uct_idx = i;
				}
			}
		}
		state.place(root->level_vector[max_uct_idx]->move_position);
		return Selection_Expansion(root->level_vector[max_uct_idx], state, state_flag*-1);
    }	

	double get_Q(const node* n, const int& idx){
		return (n->level_vector[idx]->win_count/n->level_vector[idx]->visit_count)+c_*sqrt(log(n->visit_count)/n->level_vector[idx]->visit_count);
	}
	
	board::piece_type Rollout(node* root, board&state){
		std::vector<int> tmp = root->empty_vector;
		std::shuffle(tmp.begin(), tmp.end(), engine);
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

private:
	std::vector<node*> update_node_vector;
	double c_;
};

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

class mcts_management : public mcts_tree{
public:
    mcts_management(const std::string& args = "") : mcts_tree(args), total_simulation_count_(100), total_simulation_time_(10005){
	if (total_simulation_count() != "100") total_simulation_count_ = stoi(total_simulation_count());
	if (total_simulation_time() != "10005"){
		total_simulation_time_ = stoi(total_simulation_time());
		rule = new time_compare(total_simulation_time_);
	}else{
		rule = new count_compare(total_simulation_count_);
	}
	engine.seed(std::chrono::system_clock::now().time_since_epoch().count());
    }

    void operator()(const board& state, node* root){
		board after;
		int simulation_count=0;
		// MCTS 
		while(true){
			after = state;
			MCTS_simulate(root, after);
			simulation_count++;
			if(rule->compare_result(simulation_count)) break;
		}
		std::cout << simulation_count << std::endl;
    }

protected:
	double time_management_bias_;
    
private:
	int total_simulation_count_;
	int total_simulation_time_;
	compare* rule;
};

class mcts_action : public basic_agent_func{
public:
	mcts_action(const std::string& args = ""): basic_agent_func(args), args_(args), thread_num_(1){
		if (thread_num() != "1") thread_num_ = stoi(thread_num());
	}
	virtual void close_episode(const std::string& flag = "") {}
	
	std::unordered_map<int, int> thread_simulate_result(const board& state){
		std::vector<std::thread> threads;
		std::vector<mcts_management> thread_mcts;
		std::vector<node*> thread_mcts_root;
		

		for(int i=0;i<thread_num_;i++){
			node* thread_root = new node(state);
			thread_mcts_root.push_back(thread_root);
			mcts_management tmp_mcts(args_);
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
	
        return action::place(max_visit_pos, who);
	}

private:
	std::string args_;
	int thread_num_;
};



class player : public basic_agent_func {
public:
	player(const std::string& args = "") : basic_agent_func(args), args_(args) {
		// std::cout << args << std::endl;
		if (name() == "mcts"){
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

private:
	std::string args_;
	agent* policy;
};



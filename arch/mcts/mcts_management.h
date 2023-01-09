#pragma once
#include "mcts_compare.h"

class mcts_tree{
public:
	mcts_tree(const board::piece_type& who, const float c) : who_(who),
		c_(0.3){
		if (c != 0.3) c_ = c;
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
		// other function uct2
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
		int win_value = who_ == losser ? 0 : 1;
        for(auto update_node : update_node_vector){
			update_node->visit_count += 1;
			update_node->win_count += win_value;
		}
		update_node_vector.clear();
    }
protected:
	std::default_random_engine engine;

private:
	board::piece_type who_;
	std::vector<node*> update_node_vector;
	double c_;
};

class mcts_management : public mcts_tree{
public:
    mcts_management(const board::piece_type& who, const int& gs, const int& tsc, const float& tst, const float& c) : mcts_tree(who, c), total_simulation_count_(100), total_simulation_time_(10005){
	if (tsc != 100) total_simulation_count_ = tsc;
	if (tst != 10005){
		total_simulation_time_ = tst;
		rule = new time_compare(total_simulation_time_, gs);
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
		// std::cout << simulation_count << std::endl;
    }
    
private:
	float total_simulation_count_;
	int total_simulation_time_;
	compare* rule;
};
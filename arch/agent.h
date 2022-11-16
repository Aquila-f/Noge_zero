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
		std::cout << args << std::endl;
		std::cout << space[0] << std::endl;
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

class mcts_action : public basic_agent_func {
public:
	mcts_action(const std::string& args = "") : basic_agent_func(args){}
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

class player : public basic_agent_func {
public:
	player(const std::string& args = "") : basic_agent_func(args) {
		std::cout << args << std::endl;
		exit(0);
		if (name() == "mcts"){
			policy = new mcts_action(args);
		}else{
			policy = new random_action(args);
		}
	}

	virtual action take_action(const board& state) {
		return policy->take_action(state);
	}

private:
	agent* policy;
};



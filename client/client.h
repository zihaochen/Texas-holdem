#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "IO.h"
#include "pot.h"
#include "common.h"
#include <string>

#define ERR_OUTPUT(...) fprintf(stderr, __VA_ARGS__)
#define DEBUG_OUTPUT(...) fprintf(stderr, __VA_ARGS__)
#define OUTPUT(...) fprintf(stdout, __VA_ARGS__)

using holdem::IO;
using holdem::Pot;

class Client {
public:
	friend class Query;

	bool isInitialized() const { return initialized; }

	Client(const std::string& host, const std::string& service);
	
	enum LOOP_RESULT {LOOP_NORMAL, LOOP_END, LOOP_MSG_ERROR};
	
	LOOP_RESULT loop();

	void show_final_stat();
private:

	int send(const char* fmt, ...);

	void receive(std::string& msg);

	bool receive(int var_cnt, const char* fmt, ...);

	void receive_names();

	void initialize_runtime_data();

	bool receive_round_init_msgs();

	LOOP_RESULT bet_loop(std::function<decision_type ()>, std::string round_name);

	void decide(const decision_type& decision, const std::string& round_name);

	void clear_single_game_stat();
	
	LOOP_RESULT showdown_loop();

	LOOP_RESULT receive_winner();

	/* GENERAL GAME INFO*/
	IO io;
	std::string login_name;
	std::vector<std::string> names;
	int my_id;
	int initial_chips;
	int round;
	bool initialized;

	/* PLAYER INFO */
	std::vector<int> chips;
	std::vector<PLAYER_STATUS> player_status;
	std::vector<int> current_bets;
	std::vector<int> all_chips_won_in_last_game;
	
	/* ROUND INFO */
	int num_of_participating_players;
	std::vector<int> player_list;
	bool out_of_game;
	int dealer;
	std::vector<std::pair<int, int> > bet_sequence;
	std::vector<std::pair<char, char> > community_cards;
	std::vector<HANDINFO> hands;
	std::vector<Pot> pots;
	std::vector<std::vector<std::pair<int, int> > > won_chips;
	int blind;
	int last_raise_amount;
	std::pair<char, char> hole_cards[2];
	Player& player;
};

#endif		// _CLIENT_H_

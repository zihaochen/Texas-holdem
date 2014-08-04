#include "client.h"
#include "player.h"
#include <cstdarg>
#include <utility>
#include <cstdio>
#include <sstream>
#include <algorithm>

// public

Client::Client(const std::string& host, const std::string& service): 
	io(host, service), 
	login_name("uninitialized"), 
	names(), 
	my_id(-1),
	initial_chips(0),
	round(0), 
	initialized(false), 
	
	player(*new Player(*this)) {
	
	community_cards.reserve(5);

	send("login %s", player.login_name().c_str());	// no white space in name
	
	std::string confirmation;
	receive(confirmation);
	
	if (confirmation.substr(0, 6) == "login ") {
		login_name = confirmation.substr(6, -1);
		player.login_name(login_name);
		OUTPUT("[login] %s\n", login_name.c_str());
	}

	else {
		ERR_OUTPUT("[ERROR] login is not confirmed\n");
		return ;
	}

	receive_names();

	OUTPUT("[GAME INFO] number of players = %d\n", names.size());

	if (receive(1, "initial chips = %d", &initial_chips)) {
		OUTPUT("[GAME INFO] initial chips = %d\n", initial_chips);
	}
	else {
		ERR_OUTPUT("[ERROR] unexpected message, expecting initial chips\n");
		return ;
	}

	initialize_runtime_data();

	initialized = true;
	player.init();
}

void Client::initialize_runtime_data() {
	chips.resize(names.size(), initial_chips);
	player_status.resize(names.size(), NOT_ACTIONED);
	current_bets.resize(names.size(), 0);
	all_chips_won_in_last_game.resize(names.size(), 0);
}

typename Client::LOOP_RESULT Client::loop() {

	{
		std::string msg;
		receive(msg);
		if (msg == "game starts") {
			++round;
			OUTPUT("[GAME %d] game starts\n", round);
		}

		else if (msg == "game over") {
			OUTPUT("[END]\n");
			return LOOP_END;
		}
		
		else {
			ERR_OUTPUT("[ERROR] unexpected message, expecting \"game starts\", or \"game over\".\n");
			return LOOP_MSG_ERROR;
		}
	}
	

	if (!receive_round_init_msgs()) {
		return LOOP_MSG_ERROR;
	}

	int game_continue = true;

#define TEST_LOOP_RESULT(loop_result) \
	if (game_continue) {\
		switch (loop_result) { \
		case LOOP_NORMAL:	\
			break;	\
		case LOOP_END:	\
			game_continue = false;	\
			break;	\
		case LOOP_MSG_ERROR:	\
			return LOOP_MSG_ERROR;	\
		}	\
	}

	TEST_LOOP_RESULT(bet_loop(std::bind(&Player::preflop, &player), "preflop"))
	TEST_LOOP_RESULT(bet_loop(std::bind(&Player::flop, &player), "flop"))
	TEST_LOOP_RESULT(bet_loop(std::bind(&Player::turn, &player), "turn"))
	TEST_LOOP_RESULT(bet_loop(std::bind(&Player::river, &player), "river"))
#undef TEST_LOOP_RESULT

	clear_single_game_stat();

	// expect showdown
	if (game_continue) {
		if (showdown_loop() == LOOP_MSG_ERROR) {
			return LOOP_MSG_ERROR;
		}
	}

	// declare winner message should have been delivered
	if (receive_winner() == LOOP_MSG_ERROR) {
		return LOOP_MSG_ERROR;	
	}
	
	OUTPUT("[GAME %d] game ends, invoking player.game_end()\n", round);
	player.game_end();

	return LOOP_NORMAL;
}

// private 

int Client::send(const char* fmt, ...) {
	char buffer[4097];
	va_list args;
	va_start(args, fmt);
	int num_of_written = vsnprintf(buffer, 4096, fmt, args);
	
	if (num_of_written >= 4096) {
		ERR_OUTPUT("[Warning] Message (of length %d) to be sent is too long and is truncated.\n", num_of_written);
		num_of_written = 4096;
	}

	buffer[num_of_written] = '\n';
	buffer[num_of_written+1] = '\0';

	DEBUG_OUTPUT("[send] %s", buffer);

	io.send(std::string(buffer));
	va_end(args);

	return num_of_written;
}


void Client::receive(std::string& msg) {
	io.receive(msg);
	DEBUG_OUTPUT("[receive] %s\n", msg.c_str());
}

bool Client::receive(int var_cnt, const char* fmt, ...) {
	std::string msg;
	receive(msg);

	if (var_cnt == 0) {
		return strcmp(fmt, msg.c_str()) == 0;
	}

	va_list args;
	va_start(args, fmt);
	var_cnt -= vsscanf(msg.c_str(), fmt, args);
	va_end(args);

	return var_cnt == 0;
}

void Client::receive_names() {
	std::string msg;

	receive(msg);
	while (msg != "player list end") {
		names.emplace_back(msg);
		OUTPUT("[player %d] %s\n", names.size() - 1, msg.c_str());
		if (msg == login_name) my_id = names.size() - 1;

		receive(msg);
	}
}

bool Client::receive_round_init_msgs() {
	// initialization
	// chips	check
	std::fill(player_status.begin(), player_status.end(), NOT_ACTIONED);
	std::fill(current_bets.begin(), current_bets.end(), 0);
	// all_chips_won_in_last_game	check (reset in clear_in_single_game_stat)
	// num_of_participating_players	check (see below)
	// player_list	check (see below)
	// out_of_game  check (see below)
	// dealer	check (see below)
	bet_sequence.clear();
	community_cards.clear();
	//hands		check (reset in clear_single_game_stat)
	pots.clear();
	//won_chips	check (reset in clear_single_game_stat)
	// blind	check (see below)
	// last_raise_amount	check (reset in bet_loop)
	// hole_cards	check (see below)

	std::string msg;

	if (receive(1, "number of players is %d", &num_of_participating_players)) {
		OUTPUT("[GAME INFO] number of participants in this game = %d\n", num_of_participating_players);

		receive(msg);
		std::istringstream iss(msg);
		player_list.resize(num_of_participating_players);
		for (int i = 0; i < num_of_participating_players; ++i){
			iss >> player_list[i];	
			OUTPUT("[GAME INFO] participant: %s, chip = %d\n", names[player_list[i]].c_str(), chips[player_list[i]]);
		}
	}
	else {
		ERR_OUTPUT("[ERROR] unexpected message, expecting participants.\n");
		return false;
	}

	out_of_game = std::find(player_list.begin(), player_list.end(), my_id) == player_list.end();
	if (out_of_game) {
		OUTPUT("[GAME INFO] I am out of the game.\n");
	}

	if (receive(1, "small blind is %d", &blind)) {
		OUTPUT("[GAME INFO] blind is %d\n", blind);
	}

	else {
		ERR_OUTPUT("[ERROR] unexpected message, expecting the amount of small blind.\n");
	}
	
	if (receive(1, "dealer is %d", &dealer)) {
		OUTPUT("[GAME INFO] dealer is %s\n", names[dealer].c_str());
	}
	else {
		ERR_OUTPUT("[ERROR] unexpected message, expecting dealer.\n");
		return false;
	}

	int player, bet;
	if (receive(2, "player %d blind bet %d", &player, &bet)) {
		OUTPUT("[GAME INFO] small blind %d posted by %s\n", bet, names[player].c_str());
		bet_sequence.emplace_back(player, bet);
		current_bets[player] += bet;
		chips[player] -= bet;
		blind = bet;
	}
	else {
		ERR_OUTPUT("[ERROR] unexpected message, expecting small blind.\n");
		return false;
	}

	if (receive(2, "player %d blind bet %d", &player, &bet)) {
		OUTPUT("[GAME INFO] big blind %d posted by %s\n", bet, names[player].c_str());
		bet_sequence.emplace_back(player, bet);
		current_bets[player] += bet;
		chips[player] -= bet;
	}
	else {
		ERR_OUTPUT("[ERROR] unexpected message, expecting big blind.\n");
		return false;
	}

	if (!out_of_game) {
		if (receive(2, "hole card %c %c", &(hole_cards[0].first), &(hole_cards[0].second))){
			OUTPUT("[hole card] %s %s\n", rank_of(hole_cards[0].first), suit_of(hole_cards[0].second));
		}
		else {
			ERR_OUTPUT("[ERROR] unexcepted message, expecting hole card 1.\n");
			return false;
		}

		if (receive(2, "hole card %c %c", &(hole_cards[1].first), &(hole_cards[1].second))){
			OUTPUT("[hole card] %s %s\n", rank_of(hole_cards[1].first), suit_of(hole_cards[1].second));
		}
		else {
			ERR_OUTPUT("[ERROR] unexcepted message, expecting hole card 2.\n");
			return false;
		}
	}


	return true;
}

typename Client::LOOP_RESULT Client::bet_loop(std::function<decision_type ()> get_decision, std::string round_name) {
	

	// receive message the first message of this round
	{
		std::string msg;
		receive(msg);

		std::istringstream iss(msg);
		std::string first_word;
		iss >> first_word;
		
		if (first_word == round_name){
			std::string card;
			char rank, suit;
			iss >> card >> rank >> suit;

			if (card != "card") {
				ERR_OUTPUT("[ERROR] unexpected message, expecting %s card.\n", first_word.c_str());
				return LOOP_MSG_ERROR;
			}

			community_cards.emplace_back(rank, suit);
			OUTPUT("[%s] %s %s\n", round_name.c_str(), rank_of(rank), suit_of(suit));

			if (round_name == "flop") {
				for (int i = 0; i < 2; ++i) {
					receive(msg);
					std::istringstream iss(msg);
					iss >> first_word >> card >> rank >> suit;
					if (first_word != round_name || card != "card") {
						ERR_OUTPUT("[ERROR] unexpected message, expecting %s card.\n", first_word.c_str());
						return LOOP_MSG_ERROR;
					}

					community_cards.emplace_back(rank, suit);
					OUTPUT("[%s] %s %s\n", round_name.c_str(), rank_of(rank), suit_of(suit));
				}
			}

			receive(msg);
		}

		else if (first_word == "declare") {
			if (msg != "declare winner") {
				ERR_OUTPUT("[ERROR] unexpected message, expecting declare winner\n");
				return LOOP_MSG_ERROR;
			}

			return LOOP_END;
		}

		if (msg == "round starts") {
			OUTPUT("[%s] round starts\n", round_name.c_str());
		}

		else {
			ERR_OUTPUT("[ERROR] unexpected message, expecting round starts\n");
			return LOOP_MSG_ERROR;
		}
	}

	// round starts
	if (round_name != "preflop") {
		std::fill(current_bets.begin(), current_bets.end(), false);
		bet_sequence.clear();
	}
	std::replace_if(player_status.begin(), player_status.end(), [](int x)->bool{ return x != FOLDED; }, NOT_ACTIONED);
	last_raise_amount = blind >> 1;

	// chip info (To be removed)
	for (size_t i = 0; i < player_list.size(); ++i) {
		int player_id, num_chips;
		if (receive(2, "player %d has %d chips", &player_id, &num_chips)) {
			assert(chips[player_id] == num_chips);
			OUTPUT("[%s] player %s has %d chips\n", round_name.c_str(), names[player_id].c_str(), num_chips);
		}
		else {
			ERR_OUTPUT("[ERROR] unexpected message, expecting player's number of chips\n");
		
			return LOOP_MSG_ERROR; 
		}
	}

	std::string msg;
	// action
	while (true) {
		std::string player, action;
		int player_id, bet_amount;
		receive(msg);

		if (msg == "action") {
			OUTPUT("[%s] action\n", round_name.c_str());
			
			decision_type&& decision = get_decision();
			decide(decision, round_name);
		}

		else if (msg == "round ends") {
			OUTPUT("[%s] round ends\n", round_name.c_str());
			return LOOP_NORMAL;
		}

		else {
			std::istringstream iss(msg);

			if (!(iss >> player)) {
				ERR_OUTPUT("[ERROR] unexpected message, expecting players' bets or pot summary\n");

				return LOOP_MSG_ERROR;
			}

			if (player == "pot") {
				break;
			}

			if (player != "player") {
				ERR_OUTPUT("[ERROR] unexpected message, expecting players' bets\n");
				
				return LOOP_MSG_ERROR;
			}
			
			player_id = -1;	// invalid id
			if (!(iss >> player_id) || player_id < 0 || (size_t) player_id >= names.size()) {
				ERR_OUTPUT("[ERROR] unexpected message, player id %d is out of range\n", player_id);

				return LOOP_MSG_ERROR;
			}
			
			if (!(iss >> action)) {
				ERR_OUTPUT("[ERROR] unexpected message, action not found\n");

				return LOOP_MSG_ERROR;
			}

			if (action == "checks") {
				player_status[player_id] = CHECKED;	
				OUTPUT("[%s] player %s checks\n", round_name.c_str(), names[player_id].c_str());
			}

			else if (action == "bets") {
				bet_amount = -1; // invalid amount
				if (!(iss >> bet_amount) || bet_amount < 0) {
					ERR_OUTPUT("[ERROR] unexpected message, bet amount %d < 0\n", bet_amount);

					return LOOP_MSG_ERROR;
				}		

				int previous_max_bet = *std::max_element(current_bets.begin(), current_bets.end());

				current_bets[player_id] += bet_amount;
				chips[player_id] -= bet_amount;
				bet_sequence.emplace_back(player_id, bet_amount);
				player_status[player_id] = BET;

				if (chips[player_id] == 0) {
					OUTPUT("[%s] player %s goes all in (%d chips). Total bet is %d\n", round_name.c_str(), names[player_id].c_str(), bet_amount, current_bets[player_id]);
					last_raise_amount = std::max(current_bets[player_id] - previous_max_bet, last_raise_amount);
				}
				else if (current_bets[player_id] == previous_max_bet) {
					OUTPUT("[%s] player %s calls. Total bet is %d\n", round_name.c_str(), names[player_id].c_str(), current_bets[player_id]);
				}
				else {
					last_raise_amount = current_bets[player_id] - previous_max_bet;
					OUTPUT("[%s] player %s raises %d. Total bet is %d\n", round_name.c_str(), names[player_id].c_str(), last_raise_amount, current_bets[player_id]);
				}
			}

			else if (action == "folds") {
				player_status[player_id] = FOLDED;	
				OUTPUT("[%s] player %s folds\n", round_name.c_str(), names[player_id].c_str());
			}

			else {
				ERR_OUTPUT("[ERROR] unexpected message, unknown action %s\n", action.c_str());

				return LOOP_MSG_ERROR;
			}
		}
	}

	while (msg != "round ends") {
		std::string str;
		int pot_id, pot_amount;	
		std::istringstream iss(msg);

		if (!(iss >> str) || str != "pot") {
			ERR_OUTPUT("[ERROR] unexpected message, expecting pot summary\n");
			return LOOP_MSG_ERROR;
		}

		if (!(iss >> pot_id) || (size_t) pot_id > pots.size()) {
			ERR_OUTPUT("[ERROR] unexpected message, pot_id > pot.size()\n");
			return LOOP_MSG_ERROR;
		}

		if (!(iss >> str) || str != "has") {
			ERR_OUTPUT("[ERROR] unexpected message, expecting 'has'\n");
			return LOOP_MSG_ERROR;
		}

		if (!(iss >> pot_amount)) {
			ERR_OUTPUT("[ERROR] unexpected message, expecting pot amount\n");
			return LOOP_MSG_ERROR;
		}

		if (!(iss >> str >> str >> str) || str != "by") {
			ERR_OUTPUT("[ERROR] unexpected message, expecting 'chips contributed by'\n");
			return LOOP_MSG_ERROR;
		}


		Pot pot(0);
		int player;
		while ((iss >> player)) {
			pot.add(player);
		}

		if (pot.contributors().empty()) {
			ERR_OUTPUT("[ERROR] unexpected message, expecting contributor list\n");
			return LOOP_MSG_ERROR;
		}

		pot.set_chips(pot_amount / pot.contributors().size());
		
		if (pot_id + 1 == pots.size()) {
			if (!pots.back().merge(pot)) {
				ERR_OUTPUT("[ERROR] the pots should be able to be merged\n");
				return LOOP_MSG_ERROR;
			}
		}
		else {
			pots.emplace_back(std::move(pot));
		}

		receive(msg);
	}

	return LOOP_NORMAL;
}

void Client::decide(const decision_type& decision, const std::string& round_name) {
	
	switch (decision.first) {
	case CHECK:
		{
			if (!bet_sequence.empty()) {
				OUTPUT("[WARNING] CANNOT check when another player has raised\n");
			}
			else {
				OUTPUT("[%s] check\n", round_name.c_str());
			}

			send("check");
		}

		break;
	
	case FOLD:
		{
			OUTPUT("[%s] fold\n", round_name.c_str());

			send("fold");
		}

		break;
	
	case CALL:
		{
			if (bet_sequence.empty()) {
				OUTPUT("[WARNING] CANNOT call when no one has raised. check instead\n");

				send("check");
				break;
			}
			
			int to_bet = *std::max_element(current_bets.begin(), current_bets.end()) - current_bets[my_id];
			if (to_bet > chips[my_id]) {
				OUTPUT("[%s] insufficient chips to call (need %d, has %d). All in.\n", round_name.c_str(), to_bet, chips[my_id]);
				to_bet = chips[my_id];
			}

			else if (to_bet == chips[my_id]) {
				OUTPUT("[%s] all in (%d chips).\n", round_name.c_str(), to_bet);
			}

			else {
				OUTPUT("[%s] call (%d chips). \n", round_name.c_str(), to_bet);
			}

			send("bet %d", to_bet);
		}

		break;
	
	case RAISE:
		{
			
			int to_bet = *std::max_element(current_bets.begin(), current_bets.end()) - current_bets[my_id];	
			if (decision.second <= 0) {
				if (bet_sequence.empty()) {
					OUTPUT("[WARNING] CANNOT raise a non-negative quantity of chips. Check instead.\n");

					send("check");
					break;
				}

				else {
					OUTPUT("[WARNING] CANNOT raise a non-negative quantity of chips. Call instead.\n");

					decide(make_decision(CALL), round_name);
					break;
				}
			}

			else {
				to_bet += decision.second;
			}

			if (to_bet > chips[my_id]) {
				OUTPUT("[%s] insufficient chips to raise %d (need %d, has %d). All in.\n", round_name.c_str(), decision.second, to_bet, chips[my_id]);

				to_bet = chips[my_id];
			} else {
				
				if (to_bet != chips[my_id] && decision.second < last_raise_amount) {
					if (bet_sequence.empty()) {
						OUTPUT("[WARNING] CANNOT raise %d that is smaller than the last raise %d. Check instead.\n", decision.second, last_raise_amount);
						send("check");
					}

					else {
						OUTPUT("[WARNING] CANNOT raise %d that is smaller than the last raise %d. Call instead.\n", decision.second, last_raise_amount);
						decide(make_decision(CALL), round_name);
					}

					break;
				}

				if (to_bet == chips[my_id]) {
					OUTPUT("[%s] all in (%d chips).\n", round_name.c_str(), to_bet);
				}
			
				else {
					OUTPUT("[%s] raise %d (%d chips).\n", round_name.c_str(), decision.second, to_bet);
				}
			}

			send("bet %d", to_bet);
		}

		break;
	}
}

void Client::clear_single_game_stat() {
	hands.clear();
	won_chips.clear();
	std::fill(all_chips_won_in_last_game.begin(), all_chips_won_in_last_game.end(), 0);	
}

typename Client::LOOP_RESULT Client::showdown_loop() {
	{
		std::string msg;
		receive(msg);

		if (msg == "declare winner") {
			return LOOP_END;
		}

		else if (msg == "start showdown") {
			OUTPUT("[SHOWDOWN] showdown starts\n");
		}

		else{
			ERR_OUTPUT("[ERROR] unexpected message, expecting start showdown\n");
			return LOOP_MSG_ERROR;
		}
	}

	while (true) {
		std::string msg;
		receive(msg);

		if (!msg.empty()){
			if (msg[0] == 's') {
				if (msg == "showdown") {
					OUTPUT("[SHOWDOWN] showdown\n");

					hand_type &&hand = player.showdown();
					std::ostringstream oss;
					std::ostringstream local_oss;

					local_oss << "[SHOWDOWN]";
					for (const auto& card : hand) {
						oss << card.first << ' ' << card.second << ' ';
						local_oss << ' ' << rank_of(card.first) << ' ' << suit_of(card.second);
					}

					OUTPUT("%s\n", local_oss.str().c_str());
					send(oss.str().c_str());
				}
			}

			else if (msg[0] == 'p') {
				hand_type hand;
				int player, kind;
			#define READ_HAND(i) &(hand[i].first), &(hand[i].second) 
			
				int num_read = sscanf(msg.c_str(), "player %d shows %c %c %c %c %c %c %c %c %c %c, which is %d",
					&player,
					READ_HAND(0),
					READ_HAND(1),
					READ_HAND(2),
					READ_HAND(3),
					READ_HAND(4),
					&kind);

			#undef READ_HAND
			
				if (num_read != 12) {
					ERR_OUTPUT("[ERROR] unexpected message, expecting player's showdown\n");
					return LOOP_MSG_ERROR;
				}

			#define WRITE_HAND(i) rank_of(hand[i].first), suit_of(hand[i].second)
				OUTPUT("[SHOWDOWN] player %s shows %s %s %s %s %s %s %s %s %s %s, which is %s.\n",
						names[player].c_str(),
						WRITE_HAND(0),
						WRITE_HAND(1),
						WRITE_HAND(2),
						WRITE_HAND(3),
						WRITE_HAND(4),
						HAND_STRING[kind]);
			#undef WRITE_HAND
				hands.emplace_back(hand, player, kind);
			}

			else if (msg[0] == 'd') {
				if (msg == "declare winner") {
					return LOOP_NORMAL;
				}
			}
		}
	}
	
	ERR_OUTPUT("[ERROR] %s(%d): in bet_loop: should not reach here\n", __FILE__, __LINE__);
	return LOOP_MSG_ERROR;
}

typename Client::LOOP_RESULT Client::receive_winner() {
	
	for (size_t i = 0; i < pots.size(); ++i) {
		int pot_id, sharer_cnt;
		if (!receive(2, "pot %d is shared by %d players", &pot_id, &sharer_cnt) || i != (size_t) pot_id) {
			ERR_OUTPUT("[ERROR] unexpected message, expecting winner of pot %d\n", i);
			return LOOP_MSG_ERROR;
		}
		else {
			OUTPUT("[GAME STAT] pot %d with %d chips is shared by %d players\n", pot_id, pots[i].amount(), sharer_cnt);
		}
		
		const auto& contributors = pots[i].contributors();
		int amt_per_player = pots[i].amount() / contributors.size();
		for (int player : contributors) {
			all_chips_won_in_last_game[player] -= amt_per_player;
		}
		
		std::vector<std::pair<int, int> > pot_chips; 
		for (int j = 0; j < sharer_cnt; ++j) {
			int player, num_of_chips;
			if (!receive(2, "player %d gets %d chips", &player, &num_of_chips)) {
				ERR_OUTPUT("[ERROR] unexpected messag,e expecting winner of pot %d\n", i);
				return LOOP_MSG_ERROR;
			}
			
			OUTPUT("[GAME STAT] player %s gets %d chips from pot %d\n", names[player].c_str(), num_of_chips, pot_id);
			chips[player] += num_of_chips;
			pot_chips.emplace_back(player, num_of_chips);

			all_chips_won_in_last_game[player] += num_of_chips;
		}

		won_chips.emplace_back(std::move(pot_chips));
	}

	OUTPUT("[GAME STAT] overall statistics\n");
	for (int player : player_list) {
		OUTPUT("[GAME STAT] player %s wins %d chips in this game\n", names[player].c_str(), all_chips_won_in_last_game[player]);
	}

	return LOOP_NORMAL;
}

void Client::show_final_stat() {
	OUTPUT("[FINAL STAT] chips\n");
	
	for (size_t i = 0; i < names.size(); ++i) {
		OUTPUT("[FINAL STAT] %s has %d chips\n", names[i].c_str(), chips[i]);
	}
}

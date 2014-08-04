#ifndef _QUERY_H_
#define _QUERY_H_

#include "client.h"
#include "common.h"
#include "pot.h"

using holdem::Pot;

class Client;

class Query {
public:
	
	Query(const Client& client): client(client) {}

	/* INFO of all games. */
	
	const std::string& name_of(int player) const;

	int number_of_players() const;

	int my_id() const;

	int initial_chips() const;

	/* INFO of current game. */

	int number_of_participants() const;

	const std::vector<int>& participants() const;

	bool out_of_game() const;

	int dealer() const;

	int blind() const;

	const std::vector<std::pair<int, int> >& bets() const;

	const std::vector<int>& chips() const;

	int chips(int player) const;

	const std::vector<PLAYER_STATUS>& player_status() const;

	PLAYER_STATUS player_status(int player) const;

	const std::vector<int>& current_bets() const;

	int current_bets(int player) const;

	const card_type* hole_cards() const;

	const std::vector<card_type>& community_cards() const;

	const std::vector<Pot>& pots() const;

	const Pot& pot(int pot_id) const;

	const std::vector<HANDINFO>& hands() const;

	const std::vector<std::vector<std::pair<int, int> > >& won_chips_in_pots() const;

	const std::vector<int>& chips_won_in_last_game() const;


private:
	friend class Client;

	const Client& client;	
};

// inline functions
#include "query.cpp"

#endif	// _QUERY_H_

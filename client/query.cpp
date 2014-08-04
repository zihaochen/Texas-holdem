
inline const std::string& Query::name_of(int player) const {
	return client.names[player];
}

inline int Query::number_of_players() const {
	return client.names.size();
}

inline int Query::my_id() const {
	return client.my_id;
}

inline int Query::initial_chips() const {
	return client.initial_chips;
}

/* INFO of current game. */

inline int Query::number_of_participants() const {
	return client.num_of_participating_players;	
}

inline const std::vector<int>& Query::participants() const {
	return client.player_list;
}

inline bool Query::out_of_game() const {
	return client.out_of_game;
}

inline int Query::dealer() const {
	return client.dealer;
}

inline int Query::blind() const {
	return client.blind;
}

inline const std::vector<std::pair<int, int> >& Query::bets() const {
	return client.bet_sequence;
}

inline const std::vector<int>& Query::chips() const {
	return client.chips;
}

inline int Query::chips(int player) const {
	return client.chips[player];
}

inline const std::vector<PLAYER_STATUS>& Query::player_status() const {
	return client.player_status;
}

inline PLAYER_STATUS Query::player_status(int player) const {
	return client.player_status[player];
}

inline const std::vector<int>& Query::current_bets() const {
	return client.current_bets;
}

inline int Query::current_bets(int player) const {
	return client.current_bets[player];
}

inline const card_type* Query::hole_cards() const {
	return client.hole_cards;
}

inline const std::vector<card_type>& Query::community_cards() const {
	return client.community_cards;
}

inline const std::vector<Pot>& Query::pots() const {
	return client.pots;
}

inline const Pot& Query::pot(int pot_id) const {
	return client.pots[pot_id];
}

inline const std::vector<HANDINFO>& Query::hands() const {
	return client.hands;
}

inline const std::vector<std::vector<std::pair<int, int> > >& Query::won_chips_in_pots() const {
	return client.won_chips;
}

inline const std::vector<int>& Query::chips_won_in_last_game() const {
	return client.all_chips_won_in_last_game;
}

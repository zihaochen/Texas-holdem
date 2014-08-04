#ifndef _COMMON_H_
#define _COMMON_H_

#include <utility>
#include <array>

enum DECISION_VALUE { CHECK, FOLD, CALL, RAISE };
typedef std::pair<DECISION_VALUE, int> decision_type;

enum PLAYER_STATUS { NOT_ACTIONED, BET, CHECKED, FOLDED };

const int HAND_INVALID = 0;
const int HAND_HIGH_CARD = 1;
const int HAND_ONE_PAIR = 2;
const int HAND_TWO_PAIR = 3;
const int HAND_THREE_OF_A_KIND = 4;
const int HAND_STRAIGHT = 5;
const int HAND_FLUSH = 6;
const int HAND_FULL_HOUSE = 7;
const int HAND_FOUR_OF_A_KIND = 8;
const int HAND_STRAIGHT_FLUSH = 9;
const int HAND_ROYAL_FLUSH = 10;

extern const char* HAND_STRING[11]; 

typedef std::pair<char, char> card_type;
typedef std::array<card_type, 5> hand_type;
struct HANDINFO {
	HANDINFO(hand_type hand, int player, int kind):
		hand(hand), player(player), kind(kind) {}

	hand_type hand;
	int player;
	int kind;
};

inline decision_type make_decision(DECISION_VALUE decision) {
	return std::make_pair(decision, 0);
}

inline decision_type make_decision(DECISION_VALUE decision, int bet) {
	return std::make_pair(decision, bet); 
}

inline const char* rank_of(char rank) {
	switch (rank) {
	case '2':
		return "2";
	case '3':
		return "3";
	case '4':
		return "4";
	case '5':
		return "5";
	case '6':
		return "6";
	case '7':
		return "7";
	case '8':
		return "8";
	case '9':
		return "9";
	case 'T':
		return "10";
	case 'J':
		return "J";
	case 'Q':
		return "Q";
	case 'K':
		return "K";
	case 'A':
		return "A";
	default:
		return "?";
	}
}

inline const char* suit_of(char suit) {
	switch (suit) {
	case 'S':
		return "spade";
	case 'C':
		return "club";
	case 'H':
		return "heart";
	case 'D':
		return "diamond";
	default:
		return "?";
	}
}

inline const char* rank_of(const card_type& card) { return rank_of(card.first); }
inline const char* suit_of(const card_type& card) { return suit_of(card.second); }

/* forward declarations */
class Client;
class Player;
class Query;

#endif	// _COMMON_H


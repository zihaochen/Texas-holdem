#pragma once

struct Card
{
	Card() {}
	Card(char r, char s): rank(r), suit(s) {}

    char rank;
    char suit;
};

inline bool operator==(const Card& lhs, const Card& rhs) {
	return lhs.rank == rhs.rank && lhs.suit == rhs.suit;
}

inline bool operator!=(const Card& lhs, const Card& rhs) {
	return lhs.rank != rhs.rank || lhs.suit != rhs.suit;
}



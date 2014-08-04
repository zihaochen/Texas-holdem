#pragma once

struct Card
{
	Card() {}
	Card(char r, char s): rank(r), suit(s) {}

    char rank;
    char suit;
};

bool operator==(const Card& lhs, const Card& rhs) {
	return lhs.rank == rhs.rank && lhs.suit == rhs.suit;
}

bool operator!=(const Card& lhs, const Card& rhs) {
	return lhs.rank != rhs.rank || lhs.suit != rhs.suit;
}



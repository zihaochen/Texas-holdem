#pragma once
#include <algorithm>
#include <chrono>
#include <random>
#include <vector>
#include <sstream>
#include "Card.h"

namespace holdem {

std::string _111 = "A S K S T H 4 S A C T C 6 H 9 D 7 D 2 C 8 S";

class Deck {
public:
    Deck()
    {
        /*static unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        static std::default_random_engine generator(seed);

        for (char suit : std::string("CDHS"))
		for (char rank : std::string("23456789TJQKA"))
        {
            Card card;
            card.rank = rank;
            card.suit = suit;
            cards.emplace_back(card);
        }

        std::shuffle(cards.begin(), cards.end(), generator);*/

		/*std::istringstream iss(_111);
		char r, s;
		while (iss >> r){
			iss >> s;
			cards.emplace_back(Card(r, s));
		}
		
		std::reverse(cards.begin(), cards.end());*/

    }

    void burn()
    {
        cards.pop_back();
    }

    Card deal()
    {
        Card card = cards.back();
        cards.pop_back();
        return card;
    }

	void shuffle() {
		cards.clear();

        static unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        static std::default_random_engine generator(seed);

        for (char suit : std::string("CDHS"))
		for (char rank : std::string("23456789TJQKA"))
        {
            Card card;
            card.rank = rank;
            card.suit = suit;
            cards.emplace_back(card);
        }

        std::shuffle(cards.begin(), cards.end(), generator);
	}

private:

    std::vector<Card> cards;
};

}

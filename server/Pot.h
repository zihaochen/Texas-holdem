#pragma once
#include <vector>
#include <algorithm>

namespace holdem {

class Pot {
public:
    Pot(int chips) : m(), chips(chips) {}
    Pot(const Pot &o) : m(o.m), chips(o.chips) {}
    Pot(Pot &&o) : m(std::move(o.m)), chips(o.chips) {}

    void add(int player)
    {
        m.push_back(player);
    }

    int amount() const
    {
		return chips * m.size();
	}

	int set_chips(int c) {
		return chips = c;
	}

	const std::vector<int>& contributors() const
    {
		return m;
    }

	bool merge(const Pot& pot) {
		assert(std::is_sorted(m.begin(), m.end()));
		assert(std::is_sorted(pot.m.begin(), pot.m.end()));

		if (m.size() == pot.m.size() && std::equal(m.begin(), m.end(), pot.m.begin())) {
			chips += pot.chips;

			return true;
		}

		return false;
	}

private:
	std::vector<int> m;
	int chips;
};

}

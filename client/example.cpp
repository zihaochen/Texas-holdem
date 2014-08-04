#include "client.h"
#include "player.h"
#include "common.h"
#include "Card.h"
#include <iostream>
#include <utility>
#include <cassert>
#include<fstream>
using namespace std;

void Player::init()
{
	srand(time(NULL));
	myId = query.my_id();
	currentBet = 0;
    round = 1;
    string tmpF = "The Joker";
	tmpF += (char)(myId - '0');
    out.open(tmpF);
}

void Player::destroy() {}

string Player::login_name() {
	string login_name = "The Joker";
	cout << "[UI] login name: ";
	return move(login_name);
}

void Player::login_name(string name) {
	cout << "[UI] succefully login as " << name << endl;
}

decision_type Player::preflop()
{
    own.clear();
	own.push_back(*(query.hole_cards() + 0));
	own.push_back(*(query.hole_cards() + 1));
    if (query.chips(query.my_id()) == 0) return make_decision(CALL);
	assert(own.size() == 2);
	totalBet = 0;
	findCurrentBet();
	decision_type tmp = FCR();
	out << "=======================PREFLOP=================\n";
	if (tmp.first == CHECK) out << "CHECK" << endl;
	if (tmp.first == CALL) out << "CALL" << endl;
	if (tmp.first == RAISE) out << "RAISE" << endl;
	out << "=============================================\n";
	if (tmp.first == CHECK && currentBet == 0)
	// && myId == (query.dealer() + 2) % query.number_of_participants())
		return make_decision(CALL);
	return tmp;
}

decision_type Player::flop()
{
    if (query.chips(query.my_id()) == 0) return make_decision(CALL);
	findCurrentBet();
	decision_type tmp = FCR();
	out << "=======================FLOP=================\n";
	if (tmp.first == CHECK) out << "CHECK" << endl;
	if (tmp.first == CALL) out << "CALL" << endl;
	if (tmp.first == RAISE) out << "RAISE" << endl;
	out << "=============================================\n";
	if (tmp.first == CALL && currentBet == 0)
		return make_decision(CHECK);
	return tmp;
}

decision_type Player::turn()
{
    if (query.chips(query.my_id()) == 0) return make_decision(CALL);
	findCurrentBet();
	decision_type tmp = FCR();
	out << "=======================TURN=================\n";
	if (tmp.first == CHECK) out << "CHECK" << endl;
	if (tmp.first == CALL) out << "CALL" << endl;
	if (tmp.first == RAISE) out << "RAISE" << endl;
	out << "=============================================\n";
	if (tmp.first == CALL && currentBet == 0)
		return make_decision(CHECK);
	return tmp;
}

decision_type Player::river()
{
    if (query.chips(query.my_id()) == 0) return make_decision(CALL);
	findCurrentBet();
	decision_type tmp = FCR();
	out << "=======================RIVER=================\n";
	if (tmp.first == CHECK) out << "CHECK" << endl;
	if (tmp.first == CALL) out << "CALL" << endl;
	if (tmp.first == RAISE) out << "RAISE" << endl;
	out << "=============================================\n";
	if (tmp.first == CALL && currentBet == 0)
		return make_decision(CHECK);
	return tmp;
}


#undef GET_BET_FROM_USER

hand_type Player::showdown()
{
	hand_type hand;
	vector<card_type> tmp;
	tmp = query.community_cards();
    own.clear();
	own.push_back(*(query.hole_cards() + 0));
	own.push_back(*(query.hole_cards() + 1));
	tmp.push_back(own[0]);
	tmp.push_back(own[1]);

	vector<card_type> tmpMax;
	vector<card_type> result;
	dfs(0, 0, tmp, tmpMax, result);

	hand_type temp;
	for (int i = 0; i < 5; i++)
		temp[i] = tmpMax[i];
	hand = temp;
	return move(hand);
}

void Player::game_end()
{
    round++;
}

int Player::findCurrentBet()
{
	int tmpMy = 0;
	currentBet = 0;
	vector<int> bets = query.current_bets();
	for (int i = 0; i < bets.size(); i++)
	{
		if (i == myId) tmpMy = bets[i];
		currentBet = max(currentBet, bets[i]);
	}
	currentBet -= tmpMy;
	return currentBet;
}

double Player::calHandStrength()
{
	vector<Card> cards, self, community;
	vector<card_type> comTmp;
	cards.clear(); self.clear(); community.clear();
	comTmp = query.community_cards();

	/*
	 * below are Monte Carlo simutation
	 * with the times of simutation is 1000
	 *
	 * here are the steps
	 * preparations
	 * score = 0
	 * repeat 1000 times
	 * 		shuffle the remaining cards
	 * 		if (my card is the best) score++
	 * 		else add 1/(number of people having the same strength card with me
	 * 	end repeat
	 * 	hand strength = score / 1000
	 */

	//preparations
	assert(own.size() == 2);
	for (int i = 0; i < 2; i++)
	{
		Card tmp(own[i].first, own[i].second);
		self.push_back(tmp);
	}

	for (int i = 0; i < comTmp.size(); i++)
	{
		Card tmp(comTmp[i].first, comTmp[i].second);
		community.push_back(tmp);
	}

	//create a pack of cards
    static unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    static default_random_engine generator(seed);

    for (char suit : string("CDHS"))
		for (char rank : string("23456789TJQKA"))
    	{
        	Card card;
        	card.rank = rank;
       		card.suit = suit;
			bool flag = 0;
			for (int i = 0; i < self.size(); i++)
				if (card == self[i])
				{
					flag = 1;
					break;
				}
			if (flag) continue;
			for (int i = 0; i < community.size(); i++)
				if (card == community[i])
				{
					flag = 1;
					break;
				}
			if (flag) continue;
        	cards.emplace_back(card);
    	}

	double score = 0;

    int numOfPlayers = -1;
    vector<PLAYER_STATUS> playerStatus = query.player_status();
    for (int i = 0; i < query.number_of_players(); i++)
        if (query.player_status(i)!= FOLDED) numOfPlayers++;
    out << "===============\n"
        << numOfPlayers << endl
        << "===============";

	vector<Card> otherPlayers[10];
	vector<Card> Commu;

	//Repeat 1000 times
	for (int times = 0; times < 2000; times++)
	{
		shuffle(cards.begin(), cards.end(), generator);

		Commu = community;
		for (int i = 0; i < numOfPlayers; i++)
			otherPlayers[i].clear();

		int cur = 0;
		for (int t = 0; t < 5 - community.size(); t++)
		{
			Commu.push_back(cards[cur]);
			cur++;
		}

		for (int i = 0; i < numOfPlayers; i++)
			for (int j = 0; j < 2; j++)
			{
				otherPlayers[i].push_back(cards[cur]);
				cur++;
			}

		vector<card_type> tmp;
		tmp.push_back(make_pair(self[0].rank, self[0].suit));
		tmp.push_back(make_pair(self[1].rank, self[1].suit));
		for (int i = 0; i < 5; i++)
			tmp.push_back(make_pair(Commu[i].rank, Commu[i].suit));

		vector<card_type> tmpMax;
		vector<card_type> result;
		dfs(0, 0, tmp, tmpMax, result);
		vector<card_type> Self = tmpMax;

		bool flag = 0;
		int tie = 1;
		for (int i = 0; i < numOfPlayers; i++)
		{
			vector<card_type> tmp;
			tmp.push_back(make_pair(otherPlayers[i][0].rank, otherPlayers[i][0].suit));
			tmp.push_back(make_pair(otherPlayers[i][1].rank, otherPlayers[i][1].suit));
			for (int t = 0; t < 5; t++)
				tmp.push_back(make_pair(Commu[t].rank, Commu[t].suit));
			vector<card_type> tmpMax;
			vector<card_type> result;
			dfs(0, 0, tmp, tmpMax, result);

			assert(tmpMax.size() == 5);

			if (better(tmpMax, Self) > 0)
			{
				flag = 1;
				break;
			}

			if (better(tmpMax, Self) == 0)
				tie++;
		}

		if (!flag) score += (double) 1 / tie;
	}

	//calculate HS
	double handStrength = score / 2000;

	return handStrength;
}

int Player::minRaise()
{
	// I've thought this for a long time, but still not quite sure
	vector<int> bets = query.current_bets();
	int numOfPlayers = query.number_of_participants();
	int cur = myId;
	int gap = 0, tmpMax = bets[cur] ;

	for (int i = 1; i < numOfPlayers - 1; i++)
	{
		cur = (myId + i) % numOfPlayers;
		if (query.player_status(cur) == BET && bets[cur] > tmpMax)
			gap = bets[cur] - tmpMax;
		tmpMax = max(tmpMax, bets[cur]);
	}
	if (gap == 0) gap = 2 * query.blind();
	return gap;
}

double Player::potOdds(int raise)
{
	//raise contains the amount of call
	//please make sure raise is not lower than currentBet
	double tmp;
	vector<Pot> pot = query.pots();
    vector<int> currentBets = query.current_bets();
	int totalTokens = 0;
	int myBets = 0;
	for (int i = 0; i < pot.size(); i++)
	{
		myBets += pot[i].amount() / pot[i].contributors().size();
		totalTokens += pot[i].amount();
	}
	for (int i = 0; i < currentBets.size(); i++)
    {
        if (i == myId) myBets += currentBets[i];
        totalTokens += currentBets[i];
    }
	tmp = ((double)(myBets + raise))/ (totalTokens + raise);

    out << "=========================\n";

    out << "pot size is" << pot.size()<<endl;
    out << "rasie is" << raise << endl
         << "myBets is" << myBets << endl
         << "totalTokens is" << totalTokens << endl
         << "POD is" << tmp << endl
         << "currentBet is" << currentBet << endl
         << "=========================\n";
	return tmp;
}

bool Player::randomGenerator(int percent)
{
	int tmp = rand() % 100;
	if (tmp + 1 <= percent) return 1;
	return 0;
}

decision_type Player::FCR()
{
	findCurrentBet();
	myRemainingTokens = query.chips(myId);

	double HS = calHandStrength();
	int min_raise = minRaise();
	double POD = potOdds(currentBet + min_raise);
    double PODCall = potOdds(currentBet);
	//token protection
	if ((myRemainingTokens - currentBet < query.blind() * 4) && HS < 0.65)
		return make_decision(CHECK);
    out << "=====================\n";
    for (int i =0 ;i < 2; i++)
        out << own[i].first << ' '<< own[i].second << endl;
    out << "HS is" << HS << endl;

	//decision
	double RR, RRCall;
	RRCall = HS / PODCall;
	RR = HS / POD;
	out << "RR is" << RR << endl;
	out << "RRCall is" << RRCall << endl;
    out << "=====================\n";

    if (RRCall > 1 && RR < 1) return make_decision(CALL);

    //prevent hurt from lunatic AI
    vector<Pot> pot = query.pots();
    int myBets = 0;
	for (int i = 0; i < pot.size(); i++)
		myBets += pot[i].amount() / pot[i].contributors().size();

    vector<int> currentBets = query.current_bets();
	for (int i = 0; i < currentBets.size(); i++)
        if (i == myId) myBets += currentBets[i];

	//early
	if (round <= 9)
	{
        if (myRemainingTokens > currentBet && currentBet > 5 * myBets && HS < 0.7) return make_decision(CHECK);
        if (RR < 0.8)
        {
            //if (randomGenerator(95))
                return make_decision(CHECK);
            //return make_decision(RAISE, minRaise());
        }
        if (RR < 1.0 && RRCall >= 1.2)
        {
            if (randomGenerator(60)) return (make_decision(CALL));
            if (randomGenerator(50)) return (make_decision(CHECK));
            return make_decision(RAISE, minRaise());
        }

        if (RR < 1.0 && RRCall < 1.2)
        {
            if (randomGenerator(80))
                return make_decision(CHECK);
            return make_decision(CALL);
        }
        if (RR < 1.3)
        {
            if (randomGenerator(50))
                return make_decision(CALL);
            return make_decision(RAISE, minRaise());
        }
        if (RR >= 1.3)
        {
            if (randomGenerator(20))
                return make_decision(CALL);
            return make_decision(RAISE, minRaise());
        }
    }

    //middle
    if (round > 9 && round <= 22)
	{
        if (myRemainingTokens > currentBet && currentBet > 5 * myBets && HS < 0.8) return make_decision(CHECK);
        if (RRCall > 1 && RR < 1) return make_decision(CALL);

        if (RR < 0.8)
        {
            //if (randomGenerator(95))
                return make_decision(CHECK);
            //return make_decision(RAISE, minRaise());
        }
        if (RR < 1.0 && RRCall >= 1.2)
        {
            if (randomGenerator(70)) return (make_decision(CALL));
            if (randomGenerator(40)) return (make_decision(CHECK));
            return make_decision(RAISE, minRaise());
        }

        if (RR < 1.0 && RRCall < 1.2)
        {
            if (randomGenerator(80))
                return make_decision(CHECK);
            if (randomGenerator(50))
                return make_decision(CALL);
            return make_decision(RAISE, minRaise());
        }
        if (RR < 1.3)
        {
            if (randomGenerator(50))
                return make_decision(CALL);
            return make_decision(RAISE, minRaise());
        }
        if (RR >= 1.3)
        {
            if (randomGenerator(30))
                return make_decision(CALL);
            return make_decision(RAISE, minRaise());
        }
    }

    if (round >= 23)
	{
        if (myRemainingTokens > currentBet && currentBet > 5 * myBets && HS < 0.85) return make_decision(CHECK);
        if (RRCall > 1 && RR < 1) return make_decision(CALL);

        if (RR < 0.8)
        {
            if (randomGenerator(95))
                return make_decision(CHECK);
            //return make_decision(RAISE, minRaise());
        }
        if (RR < 1.0 && RRCall >= 1.2)
        {
            if (randomGenerator(60)) return (make_decision(CALL));
            if (randomGenerator(40)) return (make_decision(CHECK));
            return make_decision(RAISE, minRaise());
        }

        if (RR < 1.0 && RRCall < 1.2)
        {
            if (randomGenerator(70))
                return make_decision(CHECK);
            if (randomGenerator(70))
                return make_decision(CALL);
            return make_decision(RAISE, minRaise());
        }
        if (RR < 1.3)
        {
            if (randomGenerator(60))
                return make_decision(CALL);
            return make_decision(RAISE, minRaise());
        }
        if (RR >= 1.3)
        {
            if (randomGenerator(50))
                return make_decision(CALL);
            return make_decision(RAISE, minRaise());
        }
    }

}

int Player::convert(char tmp)
{
	if (tmp <= '9') return (int)(tmp - '0');
	switch (tmp)
	{
		case 'T': return 10;
		case 'J': return 11;
		case 'Q': return 12;
		case 'K': return 13;
		case 'A': return 14;
	}
}

string Player::generateString(const vector<card_type> &tmp)
{
	//Straight Flush 8
	//Four of a Kind 7
	//Fullhouse 6
	//Flush 5
	//Straigh 4
	//Three of a Kind 3
	//Two Pairs 2
	//One Pair 1
	//High Card 0

	string cmp;  // size is 6, order is downward

    vector<int> tmp1;

	for (int i = 0; i < tmp.size(); i++)
		tmp1.push_back(convert(tmp[i].first));
	sort(tmp1.begin(), tmp1.end());

	//check flush
	bool flush = 1;
	for (int i = 0; i < 4; i++)
		if (tmp[i].second != tmp[i + 1].second)
		{
			flush = 0;
			break;
		}

	//check straight
	bool straight = 1;
	for (int i = 0; i < 4; i++)
		if (tmp1[i] + 1 != tmp1[i + 1])
		{
			straight = 0;
			break;
		}
	if (tmp1[0] == 2 && tmp1[1] == 3 && tmp1[2] == 4 && tmp1[3] == 5 && tmp1[4] == 14)
	{
		straight = 1;
		tmp1[0] = 14;
		for (int i = 1; i <= 4; i++)
			tmp1[i] = i + 1;
	}

	int num[20]; // times of i in tmp1
	memset(num, 0, sizeof(num));
	for (int i = 0; i < 5; i++)
		num[tmp1[i]]++;

	//check four
	bool four = 0;
	for (int i = 2; i <= 14; i++)
		if (num[i] == 4)
		{
			four = 1;
			break;
		}

	//check three
	bool three = 0;
	for (int i = 2; i <= 14; i++)
		if (num[i] == 3)
		{
			three = 1;
			break;
		}

	//count num of TWO
	int two = 0;
	for (int i = 2; i <= 14; i++)
		if (num[i] == 2)
			two++;

	//check straight flush
	if (straight && flush)
	{
		cmp += '8';
		for (int i = 4; i >= 0; i--)
			cmp += (char)(tmp1[i] + '0');
		return cmp;
	}

	//check four
	if (four)
	{
		cmp += '7';
		if (tmp1[0] != tmp1[1] && tmp1[0] != tmp1[4])
			swap(tmp1[0], tmp1[4]);
		for (int i = 0; i < 5; i++)
			cmp += (char)(tmp1[i] + '0');
		return cmp;
	}

	//check Fullhouse
	if (three && two)
	{
		cmp += '6';
		int i;
		for (i = 2; i <= 14; i++)
			if (num[i] == 3) break;
		for (int t = 0; t < 3; t++)
			cmp += (char)(i + '0');
		for (i = 2; i <= 14; i++)
			if (num[i] == 2) break;
		for (int t = 0; t < 2; t++)
			cmp += (char)(i + '0');
		return cmp;
	}

	//check Flush
	if (flush)
	{
		cmp += '5';
		for (int i = 4; i >= 0; i--)
			cmp += (char)(tmp1[i] + '0');
		return cmp;
	}

	//check straight
	if (straight)
	{
		cmp += '4';
		for (int i = 4; i >= 0; i--)
			cmp += (char)(tmp1[i] + '0');
		return cmp;
	}

	//check three
	if (three)
	{
		cmp += '3';
		int i = 2;
		for (i = 2; i <= 14; i++)
			if (num[i] == 3) break;
		for (int t = 0; t < 3; t++)
			cmp += (char)(i + '0');
		for (int t = 4; t >= 0; t--)
			if (tmp1[t] != i)
				cmp += (char)(tmp1[t] + '0');
		return cmp;
	}

	//check two pairs
	if (two == 2)
	{
		cmp += '2';
		int i;
		for (i = 14; i >= 2; i--)
			if (num[i] == 2) break;
		for (int t = 0; t < 2; t++)
			cmp += (char)(i + '0');

		int j;
		for (j = 14; j >= 2; j--)
			if (j != i && num[j] == 2) break;
		for (int t = 0; t < 2; t++)
			cmp += (char)(j + '0');

		for (int t = 4; t >= 0; t--)
			if (tmp1[t] != i && tmp1[t] != j)
				cmp += (char)(tmp1[t] + '0');
		return cmp;
	}

	//check one pair
	if (two == 1)
	{
		cmp += '1';
		int i;
		for (i = 2; i <= 14; i++)
			if (num[i] == 2) break;
		for (int t = 0; t < 2; t++)
			cmp += (char)(i + '0');
		for (int t = 4; t >= 0; t--)
			if (tmp1[t] != i)
				cmp += (char)(tmp1[t] + '0');
		return cmp;
	}

	cmp += '0';
	for (int i = 4; i >= 0; i--)
		cmp += (char)(tmp1[i] + '0');
	return cmp;
}

int Player::better(const vector<card_type> &result, const vector<card_type> &tmpMax)
{
	if (tmpMax.size() <  5) return 1;
	string tmp1 = generateString(result);
	string tmp2 = generateString(tmpMax);
	//cout << tmp1 << endl << tmp2 << endl << tmp1.compare(tmp2)<<endl;
	return (tmp1.compare(tmp2));
}

void Player::dfs(int index, int num,  const vector<card_type> &tmp, vector<card_type> &tmpMax, vector<card_type> &result)
{
	if (num == 5)
	{
		if (better(result, tmpMax) > 0) tmpMax = result;
		return;
	}
	if (index == 7) return;
	dfs(index + 1, num, tmp, tmpMax, result);
	result.push_back(tmp[index]);
	dfs(index + 1, num + 1, tmp, tmpMax, result);
	result.pop_back();
}

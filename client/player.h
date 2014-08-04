#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "query.h"
#include <algorithm>
#include <string.h>
#include <string>
#include <random>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include "Card.h"
#include <fstream>
using namespace std;

class Player {
public:

	Player(const Client& client):query(client){}

	~Player() { destroy(); }

	void init();
	void destroy();

	string login_name();	// get the login name
	void login_name(string name);	// set the login name

	int findCurrentBet();
	double calHandStrength();
	int minRaise();
	double potOdds(int);
	bool randomGenerator(int);
	decision_type FCR();
	int convert(char);
	string generateString(const vector<card_type> &);
	int better(const vector<card_type> &, const vector<card_type> &);
	void dfs(int, int, const vector<card_type> &, vector<card_type> &, vector<card_type> &);
	int findBigBlind();
	decision_type preflop();
	decision_type flop();
	decision_type turn();
	decision_type river();
	hand_type showdown();
	void game_end();
	ofstream out;

private:
	Query query;
	vector <card_type> own;
	int totalBet;
	int round;
	int currentBet;
	int myId;
	int myRemainingTokens;
};




#endif	// _PLAYER_H_

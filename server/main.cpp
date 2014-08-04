#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>
#include <clocale>
#include "log.h"
#include "Server.h"

using namespace holdem;

int main(int argc, char* argv[])
{
    if (argc < 4 || argc > 5)
    {
        std::cerr << "Usage: server <port> <numPlayers> <initialChips> [log file name]" << std::endl;
        return 1;
    }

	Log& log = Log::get_instance();
	if (argc == 5) {
		log.set_file_name(argv[4]);	
	}

    const int port = std::atoi(argv[1]);
    const int num_players = std::atoi(argv[2]);
    const int initial_chips = std::atoi(argv[3]);

    try
    {
        boost::asio::io_service io_service;
        Server s(io_service, port, num_players, initial_chips);
        io_service.run();
    }
    catch (std::exception &e)
    {
		log.detailed_out() << "[Exception] " << e.what() << std::endl;
		log.msg() << "[Exception] " << e.what() << std::endl;
        log.err() << "Exception: " << e.what() << std::endl;
		log.close_file();
    }
}

#pragma once
#include <iostream>
#include <functional>
#include <string>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include "log.h"

namespace holdem {

using boost::asio::ip::tcp;

inline std::string my_to_string(int value) {
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

class Session {
public:
    Session(boost::asio::io_service &io_service, std::function<bool(Session *)> login_callback)
        : io_service_(io_service),
          socket_(io_service),
          login_callback_(login_callback)
    {
    }

    tcp::socket &socket()
    {
        return socket_;
    }

    void start()
    {
        boost::asio::async_read_until(socket_, login_buf_, "\n",
            boost::bind(&Session::handle_login, this, boost::asio::placeholders::error));
    }

    std::string login_name() const
    {
        return login_name_;
    }

    void send(const std::string &message)
    {
        boost::asio::write(socket_, boost::asio::buffer(message.data(), message.size()));
    }

    void receive(std::string &message)
    {
        boost::asio::read_until(socket_, buf, '\n');
        std::istream is(&buf);
        std::getline(is, message);
    }

private:
    void handle_login(const boost::system::error_code &error)
    {
		Log& log = Log::get_instance();

        if (!error)
        {
            std::istream is(&login_buf_);
            std::string msg;
			std::getline(is, msg);
            
			if (msg.substr(0, 6) == "login ")
            {
                login_name_ = msg.substr(6, -1) + "_" + my_to_string(game_num) + "_" + my_to_string(player_num++);
                if (login_callback_(this))
                {
                    log.out() << "[login] " << login_name_ << std::endl;
					send("login " + login_name_ + "\n");	// confirmation
                }
                else
                {
                    log.err() << "Session handle_login: game is full" << std::endl;;
                    delete this;
                }
            }
            else
            {
                log.err() << "Session handle_login: login command expected" << std::endl;
                delete this;
            }
        }
        else
        {
            log.err() << "Session handle_login error: " << error.message() << std::endl;
            delete this;
        }
    }

    boost::asio::io_service &io_service_;
    tcp::socket socket_;
    std::function<bool(Session *)> login_callback_;
    boost::asio::streambuf login_buf_;
    std::string login_name_;

    boost::asio::streambuf buf;

public:
	static int game_num;
    static int player_num;
};

}

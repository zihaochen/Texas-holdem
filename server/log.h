#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <fstream>
#include <ctime>
#include <string>

struct Log_Output {
	Log_Output(std::ostream* the_out) : the_out(the_out) {}
	std::ostream* the_out;
};

inline Log_Output& operator<<(Log_Output& out, std::ostream& (*func)(std::ostream&)) {
	func(std::cout);
	if (out.the_out) {
		func(*out.the_out); 
	}

	return out;
}

template<class T>
inline Log_Output& operator<<(Log_Output& out, T obj) {
	std::cout << obj;
	if (out.the_out) {
		(*out.the_out) << obj;
	}

	return out;
}

class Log {
public:
	Log_Output& out() {
		return out_output;
	}

	std::ostream& err() {
		return std::cerr;
	}

	std::ostream& msg() {
		return *the_msg;
	}

	std::ostream& detailed_out() {
		return *the_out;
	}
	
	void set_file_name(const char* the_file_name) {
		log_file_name = the_file_name;
	}

	void new_file() {
		std::time_t t = std::time(nullptr);
		char strbf[100];
		std::strftime(strbf, sizeof(strbf), "%Y-%m-%d-%H-%M-%S", std::localtime(&t));
		
		if (!log_file_name.empty()) {

			the_out = new std::ofstream(log_file_name + "_" + strbf + ".log");
			the_msg = new std::ofstream(log_file_name + "_" + strbf + "_msg.log");
			out_output.the_out = the_out;
		}

		out() << "[time] " << strbf << std::endl;
		out() << ">=========================================>"  << std::endl;
	}

	void close_file() {
		
		the_out->flush();
		the_msg->flush();
		if (!log_file_name.empty()) {
			
			((std::ofstream*)the_out) -> close();
			((std::ofstream*)the_msg) -> close();
		}

		the_out = &std::cout;
		the_msg = &std::cerr;
		out_output.the_out = nullptr;
		
		std::cout << ">=========================================>"  << std::endl;
	}

private:
	std::string log_file_name;
	
	std::ostream* the_out;
	std::ostream* the_msg;
	Log_Output out_output;

	Log() : log_file_name(""), the_out(&std::cout), the_msg(&std::cerr), out_output(nullptr) {}

public:
	
	static Log& get_instance() {
		static Log log;
		return log;
	}
};

#endif // LOG_H

#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

// USED BY TESTING CLASS
#ifdef TEST_SERVER
#include "gtest/gtest_prod.h"
#endif

#include <map>
#include <vector>
#include <iostream>
#include <string>
#include <memory>
#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>

#include "config_parser.h"
#include "request_handler.h"

class ServerConfig 
{
public:
  ServerConfig() : port_(-1), num_threads_(-1) {}
  ~ServerConfig(); 
  bool parse_config(const char* arg);
  bool build_handlers(); 
  int get_port();
  int get_num_threads();
  std::string handler_map_content();
  std::shared_ptr<RequestHandler> get_handler(std::string uri); 
  std::string find_longest_matching_prefix(std::string uri);


private: 
  NginxConfig* config_; 
  int port_;
  int num_threads_;
  std::map<std::string, std::shared_ptr<RequestHandler>> handler_map_; 
  bool well_formed_uri(std::string); 
  bool get_handler_config(std::shared_ptr<NginxConfigStatement>& statement, std::unique_ptr<NginxConfig>& handler_config);

  #ifdef TEST_SERVER
  FRIEND_TEST(WellFormedURITest, ValidURI);
  FRIEND_TEST(WellFormedURITest, InvalidURI);
  #endif  
};


#endif
#include "request_handler.h"
#include "http_constants.h"
#include "status_count.h"

#include <sstream>

std::map<std::string, RequestHandler* (*)(void)>* request_handler_builders = nullptr;

RequestHandler* RequestHandler::CreateByName(const char* type) {
  const auto type_and_builder = request_handler_builders->find(type);
  if (type_and_builder == request_handler_builders->end()) {
    return nullptr;
  }
  return (*type_and_builder->second)();
}

/** ECHO HANDLER **/
RequestHandler::Status EchoHandler::Init(const std::string& uri_prefix, const NginxConfig& config) {
  uri_ = uri_prefix; 
  return OK;
}

RequestHandler::Status EchoHandler::HandleRequest(const Request& request, Response* response) {
  if (response == nullptr) {
    return INVALID_RESPONSE;
  }
  response->SetStatus(Response::OK); 
  response->AddHeader(CONTENT_TYPE, request.mime_type()); 
  response->SetBody(request.raw_request());
  return OK; 
}

std::string EchoHandler::GetName() {
  return "EchoHandler";
}

/** STATIC FILE HANDLER */
RequestHandler::Status StaticHandler::Init(const std::string& uri_prefix, const NginxConfig& config) {
  uri_ = uri_prefix; 
  for (unsigned int i = 0; i < config.statements_.size(); i++) {
    std::vector<std::string> token_list = config.statements_[i]->tokens_; 
    if (token_list.size() < 2) {
      BOOST_LOG_TRIVIAL(warning) << token_list[0] << " missing value. Ignoring statement"; 
      continue;
    }

    std::string token = token_list[0]; 
    std::string value = token_list[1];

    if (token == ROOT) {
      boost::filesystem::path p(value);
      if (boost::filesystem::exists(p) && boost::filesystem::is_directory(p)) {
        root_ = value;
        return OK; 
      }
      else {
        BOOST_LOG_TRIVIAL(warning) << value << " is an invalid path or not a directory."; 
        return INVALID_PATH; 
      }
    }
  }
  return MISSING_ROOT;
}

RequestHandler::Status StaticHandler::HandleRequest(const Request& request, Response* response) {
  if (response == nullptr) {
    return INVALID_RESPONSE;
  }

  response->SetStatus(Response::OK); 
  response->AddHeader(CONTENT_TYPE, request.mime_type());

  std::string file_path = root_ + "/" + request.file(); 

  BOOST_LOG_TRIVIAL(debug) << "File path to be opened: " << file_path << std::endl;

  // Verify if file exists:
  boost::filesystem::path p(file_path);
  if (boost::filesystem::exists(p) && !boost::filesystem::is_directory(p)) {
    // Attempt to open file:
    boost::filesystem::ifstream* file_stream = new boost::filesystem::ifstream(p);
    if (file_stream == nullptr || !file_stream->is_open()) {
      // TODO: For now it will be handled by 404, by this is better as a 500 Internal Service Error
      BOOST_LOG_TRIVIAL(warning) << "The file at " << file_path << "does not exist or is unabled to be opened"; 
      return FILE_NOT_FOUND; 
    }

    // Attempt to read in file and write to body string
    char buffer[512]; 
    std::string body = "";
    while(file_stream->read(buffer, sizeof(buffer)).gcount() > 0) {
      body.append(buffer, file_stream->gcount());
    }

    response->SetBody(body); 
    //TODO: Potentially have some checks here? 
  }
  else {
    return FILE_NOT_FOUND; 
  }

  return OK; 
}

RequestHandler::Status NotFoundHandler::Init(const std::string& uri_prefix, const NginxConfig& config) {
  uri_ = uri_prefix;
  return OK;
}

RequestHandler::Status NotFoundHandler::HandleRequest(const Request& request, Response* response) {

  std::string body = "<html><body><h1>404 Not Found</h1></body></html>"; 
  response->SetStatus(response->ResponseCode::NOT_FOUND);
  response->ClearHeaders();
  response->AddHeader("Content-Type", "text/html");
  response->SetBody(body);

  BOOST_LOG_TRIVIAL(info) << response->ToString();

  return OK;
}

std::string NotFoundHandler::GetName() {
  return "NotFoundHandler";
}
  
std::string StaticHandler::GetName() {
  return "StaticHandler";
}

/* STATUS HANDLER */

RequestHandler::Status StatusHandler::Init(const std::string& uri_prefix, const NginxConfig& config){
  uri_ = uri_prefix;
  BOOST_LOG_TRIVIAL(info) << "Called status handler Init";
  return RequestHandler::Status::OK;
}

RequestHandler::Status StatusHandler::HandleRequest(const Request& request, Response* response) {

  BOOST_LOG_TRIVIAL(info) << "Called status handler handle request";

  // get uri to handler map
  // server config has this map

  response->SetStatus(Response::ResponseCode::OK);
  response->AddHeader(CONTENT_TYPE, HTML);

  // Create Body

  std::string body = "<h4>Number of total requests: " + std::to_string(StatusCount::get_instance().request_count_);

  body += "</h4>\n\n";

  body += "<h4>Request Handlers in use: </h4>";

  for(auto const &i : StatusCount::get_instance().handlers_map_) {
    body += "<h5>" + i.first + " " + i.second + "</h5>";
  }

  body += "<h4>Status Codes: <h4>\n\n";

  for(auto const &it : StatusCount::get_instance().statuses_map_){
    body += "<h5>";
    body += it.first;
    for(auto const &j : it.second){
      body += " " + std::to_string(j.first) + ": " + std::to_string(j.second) + "\n";
    }
    body += "</h5>"; 
  }

  response->SetBody(body);

  return RequestHandler::Status::OK;
}

std::string StatusHandler::GetName() {
  return "StatusHandler";
}

/* PROXY HANDLER*/

RequestHandler::Status ProxyHandler::Init(const std::string& uri_prefix, const NginxConfig& config) {
  uri_ = uri_prefix;
  bool flg_has_host = false;
  for (unsigned int i = 0; i < config.statements_.size(); i++) {
    std::vector<std::string> token_list = config.statements_[i]->tokens_; 
    if (token_list.size() < 2) {
      BOOST_LOG_TRIVIAL(warning) << token_list[0] << " missing value. Ignoring statement"; 
      continue;
    }

    std::string token = token_list[0]; 
    std::string value = token_list[1];

    if (token == "host") {
      root_ = value; // root url for proxy e.g. ucla.edu
      flg_has_host = true;
    }

    if (token == "port"){
      port_ = value;
    }
  }

  if (not flg_has_host){
    return MISSING_ROOT;
  }
  return OK; 
}

RequestHandler::Status ProxyHandler::HandleRequest(const Request& request, Response* response) {
  BOOST_LOG_TRIVIAL(info) << "Called proxy handler: " + request.uri();
  boost::system::error_code ec;

  boost::asio::io_service svc;
  boost::asio::ip::tcp::socket sock(svc);
  boost::asio::ip::tcp::resolver resolver(svc);

  std::string req = request.raw_request();
  std::string res;
  bool flg_302 = false;

  do{
    // connect
    boost::asio::ip::tcp::resolver::iterator endpoint = resolver.resolve(boost::asio::ip::tcp::resolver::query(root_, port_));
    boost::asio::connect(sock, endpoint);
    BOOST_LOG_TRIVIAL(debug) << "Resolved IP: " << sock.remote_endpoint().address().to_string();

    // send request
    size_t h = req.find("Host");
    size_t end = req.find("\n", h);
    req.replace(h, end - h, "Host: " + root_);
    BOOST_LOG_TRIVIAL(debug) << "Proxy request: \n" << req << "=============";
    sock.send(boost::asio::buffer(req));

    // read response
    res = "";
    do {
        char buf[1024];
        size_t bytes_transferred = sock.receive(boost::asio::buffer(buf), {}, ec);
        if (!ec) {
          res.append(buf, buf + bytes_transferred);
        }
    } while (!ec);

    BOOST_LOG_TRIVIAL(debug) << "Proxy response: \n" << res << "=============";
    //check for 302
    if (res.find("HTTP/1.1 302 Found") != std::string::npos){
      flg_302 = true;
      BOOST_LOG_TRIVIAL(debug) << "Received 302";
      //find new redirect location
      size_t loc = res.find("Location");
      size_t www = res.find("www", loc);
      size_t slash = res.find("/", www);
      root_ = res.substr(www, slash - www);
      BOOST_LOG_TRIVIAL(debug) << "New location: " << root_;
    }
    else{
      flg_302 = false;
    }
  }while(flg_302);

  response->SetResponseMsg(res);

  return RequestHandler::Status::OK;
}

std::string ProxyHandler::GetName(){
  return "ProxyHandler";
}
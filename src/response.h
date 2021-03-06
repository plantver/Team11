#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>
#include <vector>


class Response {
  public:
    enum ResponseCode {
      OK = 200,
      INTERNAL_SERVER_ERROR = 500,
      BAD_REQUEST = 400,
      FORBIDDEN = 403,
      NOT_FOUND = 404
    };
  
    Response() : response_body_(""), response_msg_("") {}
    void SetStatus(const ResponseCode response_code);
    void AddHeader(const std::string& header_name, const std::string& header_value);
    void ClearHeaders(); 
    void SetBody(const std::string& body);
    int GetStatus();
    void SetResponseMsg(std::string m){ response_msg_ = m; }
    std::string ToString();

  private:
    ResponseCode status_;
    std::string response_body_;
    std::vector<std::pair<std::string, std::string>> headers_container_;
    std::string response_msg_;

    //Helper function
    std::string getTextForEnum( int enumVal );
};

#endif
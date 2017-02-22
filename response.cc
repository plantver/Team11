#include "response.h"
#include "http_constants.h"

void Response::SetStatus(const ResponseCode response_code)
{
  status_ = response_code;
}


void Response::SetBody(const std::string& body)
{
  response_body_ = body;
}

void Response::AddHeader(const std::string& header_name, const std::string& header_value)
{
  std::pair<std::string, std::string> header(header_name, header_value);
  headers_container_.push_back(header);
}

std::string Response::ToString()
{
  std::string response_header = "";
  response_header += "HTTP/1.1 " + getTextForEnum(status_) + CRLF;
  for (unsigned int i = 0; i < headers_container_.size(); ++i)
  {
    response_header += headers_container_[i].first;
    response_header += NAME_VALUE_SEPARATOR;
    response_header += headers_container_[i].second;
    response_header += CRLF;
  }

  response_header += CRLF;
  response_header += response_body_;

  return response_header; 
}

//helper function
std::string Response::getTextForEnum( int enumVal )
{
  switch( enumVal )
  {
    case ResponseCode::OK:
      return HTTP_OK;
    case ResponseCode::INTERNAL_SERVER_ERROR:
      return HTTP_INTERNAL_SERVER_ERROR;
    case ResponseCode::NOT_FOUND:
      return HTTP_NOT_FOUND;
    case ResponseCode::FORBIDDEN:
      return HTTP_FORBIDDEN;
    case ResponseCode::BAD_REQUEST:
      return HTTP_BAD_REQUEST;

    default:
      return "NOT RECOGNIZED";
  }
}
  
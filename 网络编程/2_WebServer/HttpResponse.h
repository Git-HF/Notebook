#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <unordered_map>

using std::string;
using std::unordered_map;

class HttpResponse
{

    //friend void returnResponse(const HttpResponse& response);
    
private:
    /* data */
    string m_version;
    string m_state;
    string m_state_description;
    unordered_map<string, string> m_heads;
    string m_body;

public:
    HttpResponse(/* args */);
    ~HttpResponse();
    string getVersion() const { return m_version; }
    string getState() const { return m_state; }
    string getStateDescription() const { return m_state_description; }
    unordered_map<string, string> getHeads() const { return m_heads; }
    string getBody() const { return m_body; }

    void setVersion(const string& version) { m_version = version; }
    void setState(const string& state) { m_state = state; }
    void setStateDescription(const string& state_description) { m_state_description = state_description; }
    void setHead(const string& key, const string& value) { m_heads[key] = value; }
    void setBody(const char* file_name);
    void setDefaultResponse();

    string returnResponse();
};

#endif
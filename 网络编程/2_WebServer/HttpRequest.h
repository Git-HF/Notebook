#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <unordered_map>

using std::string;
using std::unordered_map;

class HttpRequest
{
    public:
    //解析HTTP请求时，希望表示当前希望读取HTTP请求的哪一部分。
    //EXPECT_ALL表示已经完整的读取完一个HTTP请求
    enum HttpRequestParseState
    {
        EXPECT_RequestLine,
        EXPECT_Heads,
        EXPECT_Body,
        EXPECT_ALL
    };
    private:
        string m_method;
        string m_url;
        string m_version;
        unordered_map<string, string> m_heads;
        string m_body;

        HttpRequestParseState m_state;

    public:
        HttpRequest();
        ~HttpRequest();
        bool parse(string & strbuf);            //这里必须是引用，因为parse中可能读取了一部分内容，然后将剩下内容返回。
        bool is_got_all() const { return m_state == EXPECT_ALL; }
        void reset();
        void output();

};


#endif
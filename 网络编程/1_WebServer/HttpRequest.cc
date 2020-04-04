#include "HttpRequest.h"
#include <algorithm>
#include <iostream>
#include <cstring>

//static char CRLF[] = "\r\n";
static string CRLF = "\r\n";

HttpRequest::HttpRequest(/* args */) : m_state(EXPECT_RequestLine)
{
}

HttpRequest::~HttpRequest()
{
}

bool HttpRequest::parse(string &strbuf)
{
    //string tmpString = m_preString.append(buf, buf+len);
    
    bool hasMore = true;
    while (true)
    {
        string::iterator pCRLF = std::search(strbuf.begin(), strbuf.end(), CRLF.begin(), CRLF.end());
        //string ::iterator preCRLF = pCRLF - 1;
        if((pCRLF == strbuf.end()))
        {
            return true;
        }

        if(m_state == EXPECT_RequestLine) 
        {
            string::iterator pSpace = std::find(strbuf.begin(), pCRLF, ' ');
            if(pCRLF == pSpace)
                return false;
            m_method.append(std::move(strbuf.begin()), std::move(pSpace));

            string::iterator beg = ++pSpace;
            pSpace = std::find(beg, pCRLF, ' ');
            if(pCRLF == pSpace)
                return false;
            m_url.append(std::move(beg), std::move(pSpace));

            if(++pSpace == pCRLF)
                return false;
            m_version.append(std::move(pSpace), std::move(pCRLF));

            m_state = EXPECT_Heads;

            strbuf.erase(strbuf.begin(), pCRLF+2);
        }
        else if(m_state == EXPECT_Heads)
        {
            if(pCRLF == strbuf.begin())     //空行
            {
                m_state = EXPECT_ALL;           //为处理后面的body;
                strbuf.erase(strbuf.begin(), pCRLF+2);
            }
            else
            {
                string::iterator p = std::find(strbuf.begin(), pCRLF, ':');
                if(p == strbuf.end())
                    return false;
                string first(strbuf.begin(), p);
                string second(p+1, pCRLF);
                m_heads[first] = second;

                strbuf.erase(strbuf.begin(), pCRLF+2);
            }
        }
        else if(m_state == EXPECT_Body)
        {
            

        }

    }
    
}

void HttpRequest::reset()
{
    m_version.clear();
    m_method.clear();
    m_url.clear();
    m_heads.clear();
    m_body.clear();
    m_state = EXPECT_RequestLine;
}

void HttpRequest::output()
{
    printf("%s %s %s\n", m_method.c_str(), m_url.c_str(), m_version.c_str());
    for(const auto& item : m_heads)
    {
        printf("%s:%s\n", item.first.c_str(), item.second.c_str());
    }

    if(m_body.size() != 0)
    {
        printf("%s\n", m_body.c_str());
    }
}
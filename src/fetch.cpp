#include "fetch.hpp"
#include <cstring>
#include <curl/curl.h>

static size_t callback(void *ptr, size_t size, size_t nmemb, void *str)
{
    size_t real_size = size * nmemb;

    std::string in;
    in.resize(real_size);
    memcpy(&in[0], ptr, real_size);

    std::string *output = (std::string *)str;
    *output += in;

    return real_size;
}

std::string fetch(std::string url)
{
    std::string output;

    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if(!curl) throw "Unable to initialize CURL";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
        std::string error = "curl_easy_perform() failed: " + std::string(curl_easy_strerror(res));
        throw error;
    }

    curl_easy_cleanup(curl);

    return output;
}

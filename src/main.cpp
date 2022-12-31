#include <pugixml.hpp>
#include <curl/curl.h>
#include <libgssdp/gssdp.h>
#include <gio/gio.h>
#include <iostream>
#include <fstream>
#include <vector>

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

std::vector<std::string> explode(char delim, std::string str)
{
    std::vector<std::string> parts;
    std::string temp;
    size_t i = 0;
    while(i < str.size())
    {
        if(str[i] == delim)
        {
            parts.push_back(temp);
            temp = "";
        }
        else
        {
            temp += str[i];
        }
        i++;
    }
    if(temp.size()) parts.push_back(temp);
    return parts;
}

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        std::cout << "Usage:" << std::endl;
        std::cout << "    " << argv[0] << " {network interface} {Description URL}" << std::endl;
        return -1;
    }

    std::string iface = argv[1];

    std::string url = argv[2];
    std::string description;
    try {
        description = fetch(url);
    } catch (std::string err) {
        std::cerr << err << std::endl;
        return -1;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(description.c_str());

    if(!result)
    {
        std::cerr << result.description() << std::endl;
        return -1;
    }

    std::string udn = doc.child("root").child("device").child("UDN").child_value();
    std::vector<std::string> parts = explode(':', udn);
    if(parts.size() < 2)
    {
        std::cerr << "UDN does not contain UUID" << std::endl;
        return -1;
    }

    std::string device_id = "uuid:" + parts[1] + "::upnp:rootdevice";

    std::cout << "Advertising " << device_id << " at " << url << std::endl;

    GError *error = nullptr;
    GSSDPClient *client = gssdp_client_new_full(iface.c_str(),
                                        nullptr,
                                        0,
                                        GSSDP_UDA_VERSION_1_0,
                                        &error);

    if(error)
    {
        std::cout << "Error creating the GSSDP client: " << error->message << std::endl;
        g_error_free (error);
        return -1;
    }

    GSSDPResourceGroup *resource_group = gssdp_resource_group_new (client);
    gssdp_resource_group_add_resource_simple
                (resource_group,
                 "upnp:rootdevice",
                 device_id.c_str(),
                 url.c_str());

    gssdp_resource_group_set_available(resource_group, TRUE);

    GMainLoop *main_loop = g_main_loop_new (nullptr, FALSE);
    g_main_loop_run(main_loop);
    g_main_loop_unref(main_loop);

    g_object_unref(resource_group);
    g_object_unref(client);

    return 0;
}

#include <pugixml.hpp>
#include <libgssdp/gssdp.h>
#include <gio/gio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <systemd/sd-daemon.h>
#include <systemd/sd-journal.h>
#include "fetch.hpp"
#include "json.hpp"

sd_journal *journal = nullptr;

static void reload(int sig)
{
    fprintf(stderr, SD_NOTICE "ssdp-advertiser is reloading.\n");
    sd_notifyf(0, "RELOADING=1\n"
               "STATUS=Reloading Configuration\n"
               "MAINPID=%lu",
               (unsigned long) getpid());

    sd_notify(0, "READY=1\nSTATUS=Ready\n");
}

static void stop(int sig)
{
    fprintf(stderr, SD_NOTICE "ssdp-advertiser service is stopping.\n");
    sd_notify(0, "STOPPING=1");
    sd_journal_close(journal);
    exit(0);
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
    if(signal(SIGHUP, reload) == SIG_ERR)
    {
        sd_notifyf(0, "STATUS=Failed to install signal handler for service reload %s\n"
                  "ERRNO=%i",
                  strerror(errno),
                  errno);
    }

    if(signal(SIGTERM, stop) == SIG_ERR)
    {
      sd_notifyf(0, "STATUS=Failed to install signal handler for stopping service %s\n"
                  "ERRNO=%i",
                  strerror(errno),
                  errno);
    }

    sd_journal_open(&journal, 0);
    fprintf(stderr, SD_NOTICE "ssdp-advertiser service started.\n");

    nlohmann::json config;
    std::ifstream config_file("/etc/ssdp-advertiser.json");

    if(!config_file.is_open())
    {
        fprintf(stderr, SD_ERR "Couldn't open /etc/ssdp-advertiser.json\n");
        stop(0);
    }

    config = nlohmann::json::parse(config_file);

    if(config["networkInterface"].is_null())
    {
        fprintf(stderr, SD_ERR "Did not specify 'networkInterface' in /etc/ssdp-advertiser.json\n");
        stop(0);
    }

    std::string iface = config["networkInterface"].get<std::string>();

    if(config["descriptionUrl"].is_null())
    {
        fprintf(stderr, SD_ERR "Did not specify 'descriptionUrl' in /etc/ssdp-advertiser.json\n");
        stop(0);
    }

    std::string url = config["descriptionUrl"].get<std::string>();

    std::string description;
    try {
        description = fetch(url);
    } catch (std::string err) {
        fprintf(stderr, SD_ERR "%s\n", err.c_str());
        stop(0);
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(description.c_str());

    if(!result)
    {
        fprintf(stderr, SD_ERR "%s\n", result.description());
        stop(0);
    }

    std::string udn = doc.child("root").child("device").child("UDN").child_value();
    std::vector<std::string> parts = explode(':', udn);
    if(parts.size() < 2)
    {
        fprintf(stderr, SD_ERR "UDN does not contain UUID");
        stop(0);
    }

    std::string device_id = "uuid:" + parts[1] + "::upnp:rootdevice";

    fprintf(stderr, SD_NOTICE "Advertising %s at %s.\n", device_id.c_str(), url.c_str());

    GError *error = nullptr;
    GSSDPClient *client = gssdp_client_new_full(iface.c_str(),
                                        nullptr,
                                        0,
                                        GSSDP_UDA_VERSION_1_0,
                                        &error);

    if(error)
    {
        fprintf(stderr, SD_ERR "Error creating the GSSDP client: %s\n", error->message);
        g_error_free (error);
        stop(0);
    }

    GSSDPResourceGroup *resource_group = gssdp_resource_group_new (client);
    gssdp_resource_group_add_resource_simple
                (resource_group,
                 "upnp:rootdevice",
                 device_id.c_str(),
                 url.c_str());

    gssdp_resource_group_set_available(resource_group, TRUE);

    GMainLoop *main_loop = g_main_loop_new (nullptr, FALSE);

    sd_notify(0, "READY=1");

    g_main_loop_run(main_loop);
    g_main_loop_unref(main_loop);

    g_object_unref(resource_group);
    g_object_unref(client);

    return 0;
}

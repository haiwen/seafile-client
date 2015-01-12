#include "ext-common.h"
#include "log.h"
#include "applet-connection.h"

#include "commands.h"

namespace seafile {

AppletCommand::AppletCommand(std::string name)
    : name_(name)
{
}

void AppletCommand::send()
{
    std::string data = serialize();
    AppletConnection::instance()->sendCommand(data);
}

GetShareLinkCommand::GetShareLinkCommand(const std::string path)
    : AppletCommand("get-share-link"),
      path_(path)
{
}

std::string GetShareLinkCommand::serialize()
{
    return path_;
}

}

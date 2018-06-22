extern "C" {

#include <searpc.h>
#include <searpc-client.h>
#include <searpc-server.h>
#include <searpc-named-pipe-transport.h>

#include "searpc-signature.h"
#include "searpc-marshal.h"

}

#include <QCoreApplication>

#include "utils/utils.h"
#include "seafile-applet.h"
#include "configurator.h"
#include "ui/main-window.h"
#include "open-local-helper.h"

#if defined(Q_OS_WIN32)
  #include "utils/utils-win.h"
#endif

#include "rpc-server.h"


namespace {

#if defined(Q_OS_WIN32)
const char *kSeafileAppletSockName = "\\\\.\\pipe\\seafile_client_";
#else
const char *kSeafileAppletSockName = "seafile_client.sock";
#endif
const char *kSeafileAppletRpcService = "seafile-client-rpcserver";

QString getAppletRpcPipePath()
{
#if defined(Q_OS_WIN32)
    return utils::win::getLocalPipeName(kSeafileAppletSockName).c_str();
#else
    seafApplet->configurator()->checkInit();
    return QDir(seafApplet->configurator()->seafileDir()).filePath(kSeafileAppletSockName);
#endif
}

char *
handle_ping_command (GError **error)
{
    return strdup("pong");
}

int
handle_activate_command (GError **error)
{
    qWarning("[rpc server] Got an activate command");
    RpcServerProxy::instance()->proxyActivateCommand();
    return 0;
}

int
handle_exit_command (GError **error)
{
    qWarning("[rpc server] Got a quit command. Quit now.");
    RpcServerProxy::instance()->proxyExitCommand();
    return 0;
}

int
handle_open_seafile_url_command (const char *url, GError **error)
{
    qWarning("[rpc server] opening seafile url %s", url);
    RpcServerProxy::instance()->proxyOpenSeafileUrlCommand(QUrl::fromEncoded(url));
    return 0;
}

void register_rpc_service ()
{
    searpc_server_init ((RegisterMarshalFunc)register_marshals);

    searpc_create_service (kSeafileAppletRpcService);

    searpc_server_register_function (kSeafileAppletRpcService,
                                     (void *)handle_ping_command,
                                     "ping",
                                     searpc_signature_string__void());

    searpc_server_register_function (kSeafileAppletRpcService,
                                     (void *)handle_activate_command,
                                     "activate",
                                     searpc_signature_int__void());

    searpc_server_register_function (kSeafileAppletRpcService,
                                     (void *)handle_exit_command,
                                     "exit",
                                     searpc_signature_int__void());

    searpc_server_register_function (kSeafileAppletRpcService,
                                     (void *)handle_open_seafile_url_command,
                                     "open_seafile_url",
                                     searpc_signature_int__string());
}


SearpcClient *createSearpcClientWithPipeTransport(const char *rpc_service)
{
    SearpcNamedPipeClient *pipe_client;
    pipe_client = searpc_create_named_pipe_client(toCStr(getAppletRpcPipePath()));
    int ret = searpc_named_pipe_client_connect(pipe_client);
    SearpcClient *c = searpc_client_with_named_pipe_transport(pipe_client, rpc_service);
    if (ret < 0) {
        searpc_free_client_with_pipe_transport(c);
        return nullptr;
    }
    return c;
}

class AppletRpcClient : public SeafileAppletRpcServer::Client {
public:
    bool connect() {
        seafile_rpc_client_ = createSearpcClientWithPipeTransport(kSeafileAppletRpcService);
        if (!seafile_rpc_client_) {
            return false;
        }
        return true;
    }
    bool sendPingCommand(QString* resp) {
        GError *error = NULL;
        char *ret = searpc_client_call__string (
            seafile_rpc_client_,
            "ping",
            &error, 0);
        if (error) {
            g_error_free(error);
            return false;
        }
        if (!ret) {
            return false;
        }
        *resp = ret;
        return true;
    }

    bool sendActivateCommand() {
        GError *error = NULL;
        int ret = searpc_client_call__int (
            seafile_rpc_client_,
            "activate",
            &error, 0);
        if (error) {
            g_error_free(error);
            return false;
        }
        if (ret != 0) {
            return false;
        }
        return true;
    }
    bool sendExitCommand() {
        GError *error = NULL;
        int ret = searpc_client_call__int (
            seafile_rpc_client_,
            "exit",
            &error, 0);
        if (error) {
            g_error_free(error);
            return false;
        }
        if (ret != 0) {
            return false;
        }
        return true;
    }

    bool sendOpenSeafileUrlCommand(const QUrl& url) {
        GError *error = NULL;
        int ret = searpc_client_call__int (
            seafile_rpc_client_,
            "open_seafile_url",
            &error, 1, "string", url.toEncoded().data());
        if (error) {
            g_error_free(error);
            return false;
        }
        if (ret != 0) {
            return false;
        }
        return true;
    }
private:
    SearpcClient *seafile_rpc_client_;

};

} // namespace

struct SeafileAppletRpcServerPriv {
    SearpcNamedPipeServer *pipe_server;
};


SINGLETON_IMPL(SeafileAppletRpcServer)

SeafileAppletRpcServer::SeafileAppletRpcServer()
: priv_(new SeafileAppletRpcServerPriv)
{
    priv_->pipe_server = searpc_create_named_pipe_server(toCStr(getAppletRpcPipePath()));

    RpcServerProxy *proxy = RpcServerProxy::instance();
    connect(
        proxy, SIGNAL(activateCommand()), this, SLOT(handleActivateCommand()));
    connect(proxy, SIGNAL(exitCommand()), this, SLOT(handleExitCommand()));
    connect(proxy,
            SIGNAL(openSeafileUrlCommand(const QUrl &)),
            this,
            SLOT(handleOpenSeafileUrlCommand(const QUrl &)));
}

SeafileAppletRpcServer::~SeafileAppletRpcServer()
{
}

void SeafileAppletRpcServer::start()
{
    register_rpc_service();
    qWarning("starting applet rpc service");
    if (searpc_named_pipe_server_start(priv_->pipe_server) < 0) {
        qWarning("failed to start rpc service");
        seafApplet->errorAndExit("failed to initialize");
    } else {
        qWarning("applet rpc service started");
    }
}

SeafileAppletRpcServer::Client* SeafileAppletRpcServer::getClient()
{
    return new AppletRpcClient();
}

void SeafileAppletRpcServer::handleActivateCommand()
{
    seafApplet->mainWindow()->showWindow();
}

void SeafileAppletRpcServer::handleExitCommand()
{
    qWarning("[Message Listener] Got a quit command. Quit now.");
    QCoreApplication::exit(0);
}

void SeafileAppletRpcServer::handleOpenSeafileUrlCommand(const QUrl& url)
{
    OpenLocalHelper::instance()->openLocalFile(url);
}


SINGLETON_IMPL(RpcServerProxy)

RpcServerProxy::RpcServerProxy()
{
}

void RpcServerProxy::proxyActivateCommand()
{
    emit activateCommand();
}

void RpcServerProxy::proxyExitCommand()
{
    emit exitCommand();
}

void RpcServerProxy::proxyOpenSeafileUrlCommand(const QUrl& url)
{
    emit openSeafileUrlCommand(url);
}

#include "api-request.h"

#include "api-client.h"

SeafileApiRequest::SeafileApiRequest(const QUrl& url, Method method, const QString& token)
    : url_(url),
      method_(method),
      token_(token)
{
    api_client_ = new SeafileApiClient;
}

SeafileApiRequest::~SeafileApiRequest()
{
    delete api_client_;
}

void SeafileApiRequest::setParam(const QString& name, const QString& value)
{
    params_.addQueryItem(name, value);
}

void SeafileApiRequest::send()
{
    if (token_.size() > 0) {
        api_client_->setToken(token_);
    }

    switch (method_) {
    case METHOD_GET:
        api_client_->get(url_);
    case METHOD_POST:
        api_client_->post(url_, params_.encodedQuery());
    }

    connect(api_client_, SIGNAL(requestSuccess(QNetworkReply&)),
            this, SLOT(requestSuccess(QNetworkReply&)));

    connect(api_client_, SIGNAL(requestFailed(int)),
            this, SIGNAL(failed(int)));
}

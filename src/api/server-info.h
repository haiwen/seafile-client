#ifndef SEAFILE_CLIENT_API_SERVER_INFO_H
#define SEAFILE_CLIENT_API_SERVER_INFO_H

enum ServerExtraFeature {
  FeatureBasic = 0,
  FeatureProVersion = 0x1,
  FeatureOfficePreview = 0x2,
  FeatureFileSearch = 0x4
};

class ServerInfo {
public:
  unsigned majorVersion;
  unsigned minorVersion;
  unsigned patchVersion;
  bool pro;
  unsigned feature;
  ServerInfo() : majorVersion(0),
                 minorVersion(0),
                 patchVersion(0),
                 pro(false),
                 feature(FeatureBasic) {
  }
  ServerInfo(const ServerInfo&rhs) : majorVersion(rhs.majorVersion),
                 minorVersion(rhs.minorVersion),
                 patchVersion(rhs.patchVersion),
                 pro(rhs.pro),
                 feature(rhs.feature) {
  }
};



#endif // SEAFILE_CLIENT_API_SERVER_INFO_H

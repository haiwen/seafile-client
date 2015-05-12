#ifndef SEAFILE_CLIENT_API_SERVER_INFO_H
#define SEAFILE_CLIENT_API_SERVER_INFO_H
#include <QString>
#include <QStringList>

class ServerInfo {
public:
  unsigned majorVersion;
  unsigned minorVersion;
  unsigned patchVersion;
  bool proEdition;
  bool officePreview;
  bool fileSearch;
  ServerInfo() : majorVersion(0),
                 minorVersion(0),
                 patchVersion(0),
                 proEdition(false),
                 officePreview(false),
                 fileSearch(false) {
  }
  ServerInfo(const ServerInfo &rhs)
      : majorVersion(rhs.majorVersion), minorVersion(rhs.minorVersion),
        patchVersion(rhs.patchVersion), proEdition(rhs.proEdition), officePreview(rhs.officePreview), fileSearch(rhs.fileSearch) {}
  ServerInfo& operator=(const ServerInfo&rhs) {
      majorVersion = rhs.majorVersion;
      minorVersion = rhs.minorVersion;
      patchVersion = rhs.patchVersion;
      proEdition = rhs.proEdition;
      officePreview = rhs.officePreview;
      fileSearch = rhs.fileSearch;
      return *this;
  }
  bool operator== (const ServerInfo &rhs) const {
      return majorVersion == rhs.majorVersion &&
             minorVersion == rhs.minorVersion &&
             patchVersion == rhs.patchVersion &&
             proEdition == rhs.proEdition &&
             officePreview == rhs.officePreview &&
             fileSearch == rhs.fileSearch;
  }
  bool operator!= (const ServerInfo &rhs) const {
      return !(*this == rhs);
  }
  bool parseVersionFromString(const QString &version) {
      QStringList versions = version.split('.');
      if (versions.size() < 3)
          return false;
      majorVersion = versions[0].toInt();
      minorVersion = versions[1].toInt();
      patchVersion = versions[2].toInt();
      return true;
  }
  void parseFeatureFromStrings(const QStringList& input) {
      proEdition = false;
      officePreview = false;
      fileSearch = false;
      Q_FOREACH(const QString& key, input)
      {
          parseFeatureFromString(key);
      }
  }
  bool parseFeatureFromString(const QString& key, bool value = true) {
      if (key == "seafile-pro") {
          if (value) {
              proEdition = true;
          } else {
              proEdition = false;
          }
      } else if (key == "office-preview") {
          if (value)
              officePreview = true;
          else
              officePreview = false;
      } else if (key == "file-search") {
          if (value)
              fileSearch = true;
          else
              fileSearch = false;
      } else {
          return false;
      }
      return true;
  }
  QString getVersionString() const {
      return QString("%1.%2.%3")
          .arg(QString::number(majorVersion))
          .arg(QString::number(minorVersion))
          .arg(QString::number(patchVersion));
  }
  QStringList getFeatureStrings() const {
      QStringList result;
      if (proEdition)
          result.push_back("seafile-pro");
      if (officePreview)
          result.push_back("office-preview");
      if (fileSearch)
          result.push_back("file-search");
      return result;
  }
};



#endif // SEAFILE_CLIENT_API_SERVER_INFO_H

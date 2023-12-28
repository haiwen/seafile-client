#ifndef SEAFILE_CLIENT_API_SERVER_INFO_H
#define SEAFILE_CLIENT_API_SERVER_INFO_H
#include <QString>
#include <QStringList>

class ServerInfo {
public:
  unsigned majorVersion;
  unsigned minorVersion;
  unsigned patchVersion;
  unsigned encryptedLibraryVersion = 2;
  bool proEdition;
  bool officePreview;
  bool fileSearch;
  bool disableSyncWithAnyFolder;
  bool clientSSOViaLocalBrowser;
  QString customBrand;
  QString customLogo;

  bool operator== (const ServerInfo &rhs) const {
      return majorVersion == rhs.majorVersion &&
          minorVersion == rhs.minorVersion &&
          patchVersion == rhs.patchVersion &&
          encryptedLibraryVersion == rhs.encryptedLibraryVersion &&
          proEdition == rhs.proEdition &&
          officePreview == rhs.officePreview &&
          fileSearch == rhs.fileSearch &&
          disableSyncWithAnyFolder == rhs.disableSyncWithAnyFolder &&
          customBrand == rhs.customBrand &&
          customLogo == rhs.customLogo;
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

  bool parseEncryptedLibraryVersionFromString(const QString &version) {
      bool ok;
      encryptedLibraryVersion = version.toInt(&ok);
      return ok;
  }

  void parseFeatureFromStrings(const QStringList& input) {
      proEdition = false;
      officePreview = false;
      fileSearch = false;
      disableSyncWithAnyFolder = false;
      Q_FOREACH(const QString& key, input)
      {
          parseFeatureFromString(key);
      }
  }
  bool parseFeatureFromString(const QString& key, bool value = true) {
      if (key == "seafile-pro") {
          proEdition = value;
      } else if (key == "office-preview") {
          officePreview = value;
      } else if (key == "file-search") {
          fileSearch = value;
      } else if (key == "disable-sync-with-any-folder") {
          disableSyncWithAnyFolder = value;
      } else if (key == "client-sso-via-local-browser") {
          clientSSOViaLocalBrowser = value;
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

  int getEncryptedLibraryVersion() const {
      return encryptedLibraryVersion;
  }

  QStringList getFeatureStrings() const {
      QStringList result;
      if (proEdition)
          result.push_back("seafile-pro");
      if (officePreview)
          result.push_back("office-preview");
      if (fileSearch)
          result.push_back("file-search");
      if (disableSyncWithAnyFolder)
          result.push_back("disable-sync-with-any-folder");
      return result;
  }
};



#endif // SEAFILE_CLIENT_API_SERVER_INFO_H

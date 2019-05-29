#include <QHash>
#include <QFileInfo>
#include <QDir>
#include <QStringList>
#include "stl.h"

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

#include "file-utils.h"

namespace {

QHash<QString, QString> *types_map = 0;

void init()
{
    types_map = new QHash<QString, QString>;

    // Adapted from /etc/mime.types on ubuntu 12.04
    // TODO: on windows we should read the system registry for a more complete mime types list

    types_map->insert("%", "application/x-trash");
    types_map->insert("323", "text/h323");
    types_map->insert("3gp", "video/3gpp");
    types_map->insert("7z", "application/x-7z-compressed");
    types_map->insert("a", "application/octet-stream");
    types_map->insert("abw", "application/x-abiword");
    types_map->insert("ai", "application/postscript");
    types_map->insert("aif", "audio/x-aiff");
    types_map->insert("aifc", "audio/x-aiff");
    types_map->insert("aiff", "audio/x-aiff");
    types_map->insert("alc", "chemical/x-alchemy");
    types_map->insert("amr", "audio/amr");
    types_map->insert("anx", "application/annodex");
    types_map->insert("apk", "application/vnd.android.package-archive");
    types_map->insert("art", "image/x-jg");
    types_map->insert("asc", "text/plain");
    types_map->insert("asf", "video/x-ms-asf");
    types_map->insert("asn", "chemical/x-ncbi-asn1-spec");
    types_map->insert("aso", "chemical/x-ncbi-asn1-binary");
    types_map->insert("asx", "video/x-ms-asf");
    types_map->insert("atom", "application/atom+xml");
    types_map->insert("atomcat", "application/atomcat+xml");
    types_map->insert("atomsrv", "application/atomserv+xml");
    types_map->insert("au", "audio/basic");
    types_map->insert("avi", "video/x-msvideo");
    types_map->insert("awb", "audio/amr-wb");
    types_map->insert("axa", "audio/annodex");
    types_map->insert("axv", "video/annodex");
    types_map->insert("b", "chemical/x-molconn-Z");
    types_map->insert("bak", "application/x-trash");
    types_map->insert("bat", "application/x-msdos-program");
    types_map->insert("bcpio", "application/x-bcpio");
    types_map->insert("bib", "text/x-bibtex");
    types_map->insert("bin", "application/octet-stream");
    types_map->insert("bmp", "image/x-ms-bmp");
    types_map->insert("boo", "text/x-boo");
    types_map->insert("book", "application/x-maker");
    types_map->insert("brf", "text/plain");
    types_map->insert("bsd", "chemical/x-crossfire");
    types_map->insert("c", "text/x-csrc");
    types_map->insert("c++", "text/x-c++src");
    types_map->insert("c3d", "chemical/x-chem3d");
    types_map->insert("cab", "application/x-cab");
    types_map->insert("cac", "chemical/x-cache");
    types_map->insert("cache", "chemical/x-cache");
    types_map->insert("cap", "application/cap");
    types_map->insert("cascii", "chemical/x-cactvs-binary");
    types_map->insert("cat", "application/vnd.ms-pki.seccat");
    types_map->insert("cbin", "chemical/x-cactvs-binary");
    types_map->insert("cbr", "application/x-cbr");
    types_map->insert("cbz", "application/x-cbz");
    types_map->insert("cc", "text/x-c++src");
    types_map->insert("cda", "application/x-cdf");
    types_map->insert("cdf", "application/x-cdf");
    types_map->insert("cdr", "image/x-coreldraw");
    types_map->insert("cdt", "image/x-coreldrawtemplate");
    types_map->insert("cdx", "chemical/x-cdx");
    types_map->insert("cdy", "application/vnd.cinderella");
    types_map->insert("cef", "chemical/x-cxf");
    types_map->insert("cer", "chemical/x-cerius");
    types_map->insert("chm", "chemical/x-chemdraw");
    types_map->insert("chrt", "application/x-kchart");
    types_map->insert("cif", "chemical/x-cif");
    types_map->insert("class", "application/java-vm");
    types_map->insert("cls", "text/x-tex");
    types_map->insert("cmdf", "chemical/x-cmdf");
    types_map->insert("cml", "chemical/x-cml");
    types_map->insert("cod", "application/vnd.rim.cod");
    types_map->insert("com", "application/x-msdos-program");
    types_map->insert("cpa", "chemical/x-compass");
    types_map->insert("cpio", "application/x-cpio");
    types_map->insert("cpp", "text/x-c++src");
    types_map->insert("cpt", "image/x-corelphotopaint");
    types_map->insert("cr2", "image/x-canon-cr2");
    types_map->insert("crl", "application/x-pkcs7-crl");
    types_map->insert("crt", "application/x-x509-ca-cert");
    types_map->insert("crw", "image/x-canon-crw");
    types_map->insert("csd", "audio/csound");
    types_map->insert("csf", "chemical/x-cache-csf");
    types_map->insert("csh", "text/x-csh");
    types_map->insert("csm", "chemical/x-csml");
    types_map->insert("csml", "chemical/x-csml");
    types_map->insert("css", "text/css");
    types_map->insert("csv", "text/csv");
    types_map->insert("ctab", "chemical/x-cactvs-binary");
    types_map->insert("ctx", "chemical/x-ctx");
    types_map->insert("cu", "application/cu-seeme");
    types_map->insert("cub", "chemical/x-gaussian-cube");
    types_map->insert("cxf", "chemical/x-cxf");
    types_map->insert("cxx", "text/x-c++src");
    types_map->insert("d", "text/x-dsrc");
    types_map->insert("dat", "application/x-ns-proxy-autoconfig");
    types_map->insert("davmount", "application/davmount+xml");
    types_map->insert("dcr", "application/x-director");
    types_map->insert("deb", "application/x-debian-package");
    types_map->insert("dif", "video/dv");
    types_map->insert("diff", "text/x-diff");
    types_map->insert("dir", "application/x-director");
    types_map->insert("djv", "image/vnd.djvu");
    types_map->insert("djvu", "image/vnd.djvu");
    types_map->insert("dl", "video/dl");
    types_map->insert("dll", "application/x-msdos-program");
    types_map->insert("dmg", "application/x-apple-diskimage");
    types_map->insert("dms", "application/x-dms");
    types_map->insert("doc", "application/msword");
    types_map->insert("docm", "application/vnd.ms-word.document.macroEnabled.12");
    types_map->insert("docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document");
    types_map->insert("dot", "application/msword");
    types_map->insert("dotm", "application/vnd.ms-word.template.macroEnabled.12");
    types_map->insert("dotx", "application/vnd.openxmlformats-officedocument.wordprocessingml.template");
    types_map->insert("dv", "video/dv");
    types_map->insert("dvi", "application/x-dvi");
    types_map->insert("dx", "chemical/x-jcamp-dx");
    types_map->insert("dxr", "application/x-director");
    types_map->insert("emb", "chemical/x-embl-dl-nucleotide");
    types_map->insert("embl", "chemical/x-embl-dl-nucleotide");
    types_map->insert("eml", "message/rfc822");
    types_map->insert("ent", "chemical/x-pdb");
    types_map->insert("eps", "application/postscript");
    types_map->insert("eps2", "application/postscript");
    types_map->insert("eps3", "application/postscript");
    types_map->insert("epsf", "application/postscript");
    types_map->insert("epsi", "application/postscript");
    types_map->insert("erf", "image/x-epson-erf");
    types_map->insert("es", "application/ecmascript");
    types_map->insert("etx", "text/x-setext");
    types_map->insert("exe", "application/x-msdos-program");
    types_map->insert("ez", "application/andrew-inset");
    types_map->insert("fb", "application/x-maker");
    types_map->insert("fbdoc", "application/x-maker");
    types_map->insert("fch", "chemical/x-gaussian-checkpoint");
    types_map->insert("fchk", "chemical/x-gaussian-checkpoint");
    types_map->insert("fig", "application/x-xfig");
    types_map->insert("flac", "audio/flac");
    types_map->insert("fli", "video/fli");
    types_map->insert("flv", "video/x-flv");
    types_map->insert("fm", "application/x-maker");
    types_map->insert("fodg", "application/vnd.oasis.opendocument.graphics-flat-xml");
    types_map->insert("fodp", "application/vnd.oasis.opendocument.presentation-flat-xml");
    types_map->insert("fods", "application/vnd.oasis.opendocument.spreadsheet-flat-xml");
    types_map->insert("fodt", "application/vnd.oasis.opendocument.text-flat-xml");
    types_map->insert("frame", "application/x-maker");
    types_map->insert("frm", "application/x-maker");
    types_map->insert("gal", "chemical/x-gaussian-log");
    types_map->insert("gam", "chemical/x-gamess-input");
    types_map->insert("gamin", "chemical/x-gamess-input");
    types_map->insert("gan", "application/x-ganttproject");
    types_map->insert("gau", "chemical/x-gaussian-input");
    types_map->insert("gcd", "text/x-pcs-gcd");
    types_map->insert("gcf", "application/x-graphing-calculator");
    types_map->insert("gcg", "chemical/x-gcg8-sequence");
    types_map->insert("gen", "chemical/x-genbank");
    types_map->insert("gf", "application/x-tex-gf");
    types_map->insert("gif", "image/gif");
    types_map->insert("gjc", "chemical/x-gaussian-input");
    types_map->insert("gjf", "chemical/x-gaussian-input");
    types_map->insert("gl", "video/gl");
    types_map->insert("gnumeric", "application/x-gnumeric");
    types_map->insert("gpt", "chemical/x-mopac-graph");
    types_map->insert("gsf", "application/x-font");
    types_map->insert("gsm", "audio/x-gsm");
    types_map->insert("gtar", "application/x-gtar");
    types_map->insert("h", "text/x-chdr");
    types_map->insert("h++", "text/x-c++hdr");
    types_map->insert("hdf", "application/x-hdf");
    types_map->insert("hh", "text/x-c++hdr");
    types_map->insert("hin", "chemical/x-hin");
    types_map->insert("hpp", "text/x-c++hdr");
    types_map->insert("hqx", "application/mac-binhex40");
    types_map->insert("hs", "text/x-haskell");
    types_map->insert("hta", "application/hta");
    types_map->insert("htc", "text/x-component");
    types_map->insert("htm", "text/html");
    types_map->insert("html", "text/html");
    types_map->insert("hxx", "text/x-c++hdr");
    types_map->insert("ica", "application/x-ica");
    types_map->insert("ice", "x-conference/x-cooltalk");
    types_map->insert("ico", "image/x-icon");
    types_map->insert("ics", "text/calendar");
    types_map->insert("icz", "text/calendar");
    types_map->insert("ief", "image/ief");
    types_map->insert("iges", "model/iges");
    types_map->insert("igs", "model/iges");
    types_map->insert("iii", "application/x-iphone");
    types_map->insert("info", "application/x-info");
    types_map->insert("inp", "chemical/x-gamess-input");
    types_map->insert("ins", "application/x-internet-signup");
    types_map->insert("iso", "application/x-iso9660-image");
    types_map->insert("isp", "application/x-internet-signup");
    types_map->insert("ist", "chemical/x-isostar");
    types_map->insert("istr", "chemical/x-isostar");
    types_map->insert("jad", "text/vnd.sun.j2me.app-descriptor");
    types_map->insert("jam", "application/x-jam");
    types_map->insert("jar", "application/java-archive");
    types_map->insert("java", "text/x-java");
    types_map->insert("jdx", "chemical/x-jcamp-dx");
    types_map->insert("jmz", "application/x-jmol");
    types_map->insert("jng", "image/x-jng");
    types_map->insert("jnlp", "application/x-java-jnlp-file");
    types_map->insert("jpe", "image/jpeg");
    types_map->insert("jpeg", "image/jpeg");
    types_map->insert("jpg", "image/jpeg");
    types_map->insert("js", "application/javascript");
    types_map->insert("json", "application/json");
    types_map->insert("kar", "audio/midi");
    types_map->insert("key", "application/pgp-keys");
    types_map->insert("kil", "application/x-killustrator");
    types_map->insert("kin", "chemical/x-kinemage");
    types_map->insert("kml", "application/vnd.google-earth.kml+xml");
    types_map->insert("kmz", "application/vnd.google-earth.kmz");
    types_map->insert("kpr", "application/x-kpresenter");
    types_map->insert("kpt", "application/x-kpresenter");
    types_map->insert("ksh", "text/plain");
    types_map->insert("ksp", "application/x-kspread");
    types_map->insert("kwd", "application/x-kword");
    types_map->insert("kwt", "application/x-kword");
    types_map->insert("latex", "application/x-latex");
    types_map->insert("lha", "application/x-lha");
    types_map->insert("lhs", "text/x-literate-haskell");
    types_map->insert("lin", "application/bbolin");
    types_map->insert("lsf", "video/x-la-asf");
    types_map->insert("lsx", "video/x-la-asf");
    types_map->insert("ltx", "text/x-tex");
    types_map->insert("lyx", "application/x-lyx");
    types_map->insert("lzh", "application/x-lzh");
    types_map->insert("lzx", "application/x-lzx");
    types_map->insert("m1v", "video/mpeg");
    types_map->insert("m3g", "application/m3g");
    types_map->insert("m3u", "audio/x-mpegurl");
    types_map->insert("m3u8", "application/x-mpegURL");
    types_map->insert("m4a", "audio/mpeg");
    types_map->insert("maker", "application/x-maker");
    types_map->insert("man", "application/x-troff-man");
    types_map->insert("manifest", "text/cache-manifest");
    types_map->insert("mcif", "chemical/x-mmcif");
    types_map->insert("mcm", "chemical/x-macmolecule");
    types_map->insert("mdb", "application/msaccess");
    types_map->insert("me", "application/x-troff-me");
    types_map->insert("mesh", "model/mesh");
    types_map->insert("mht", "message/rfc822");
    types_map->insert("mhtml", "message/rfc822");
    types_map->insert("mid", "audio/midi");
    types_map->insert("midi", "audio/midi");
    types_map->insert("mif", "application/x-mif");
    types_map->insert("mkv", "video/x-matroska");
    types_map->insert("mm", "application/x-freemind");
    types_map->insert("mmd", "chemical/x-macromodel-input");
    types_map->insert("mmf", "application/vnd.smaf");
    types_map->insert("mml", "text/mathml");
    types_map->insert("mmod", "chemical/x-macromodel-input");
    types_map->insert("mng", "video/x-mng");
    types_map->insert("moc", "text/x-moc");
    types_map->insert("mol", "chemical/x-mdl-molfile");
    types_map->insert("mol2", "chemical/x-mol2");
    types_map->insert("moo", "chemical/x-mopac-out");
    types_map->insert("mop", "chemical/x-mopac-input");
    types_map->insert("mopcrt", "chemical/x-mopac-input");
    types_map->insert("mov", "video/quicktime");
    types_map->insert("movie", "video/x-sgi-movie");
    types_map->insert("mp2", "audio/mpeg");
    types_map->insert("mp3", "audio/mpeg");
    types_map->insert("mp4", "video/mp4");
    types_map->insert("mpa", "video/mpeg");
    types_map->insert("mpc", "chemical/x-mopac-input");
    types_map->insert("mpe", "video/mpeg");
    types_map->insert("mpeg", "video/mpeg");
    types_map->insert("mpega", "audio/mpeg");
    types_map->insert("mpg", "video/mpeg");
    types_map->insert("mpga", "audio/mpeg");
    types_map->insert("mph", "application/x-comsol");
    types_map->insert("mpv", "video/x-matroska");
    types_map->insert("ms", "application/x-troff-ms");
    types_map->insert("msh", "model/mesh");
    types_map->insert("msi", "application/x-msi");
    types_map->insert("mvb", "chemical/x-mopac-vib");
    types_map->insert("mxf", "application/mxf");
    types_map->insert("mxu", "video/vnd.mpegurl");
    types_map->insert("nb", "application/mathematica");
    types_map->insert("nbp", "application/mathematica");
    types_map->insert("nc", "application/x-netcdf");
    types_map->insert("nef", "image/x-nikon-nef");
    types_map->insert("nwc", "application/x-nwc");
    types_map->insert("nws", "message/rfc822");
    types_map->insert("o", "application/x-object");
    types_map->insert("obj", "application/octet-stream");
    types_map->insert("oda", "application/oda");
    types_map->insert("odb", "application/vnd.sun.xml.base");
    types_map->insert("odc", "application/vnd.oasis.opendocument.chart");
    types_map->insert("odf", "application/vnd.oasis.opendocument.formula");
    types_map->insert("odg", "application/vnd.oasis.opendocument.graphics");
    types_map->insert("odi", "application/vnd.oasis.opendocument.image");
    types_map->insert("odm", "application/vnd.oasis.opendocument.text-master");
    types_map->insert("odp", "application/vnd.oasis.opendocument.presentation");
    types_map->insert("ods", "application/vnd.oasis.opendocument.spreadsheet");
    types_map->insert("odt", "application/vnd.oasis.opendocument.text");
    types_map->insert("oga", "audio/ogg");
    types_map->insert("ogg", "audio/ogg");
    types_map->insert("ogv", "video/ogg");
    types_map->insert("ogx", "application/ogg");
    types_map->insert("old", "application/x-trash");
    types_map->insert("one", "application/onenote");
    types_map->insert("onepkg", "application/onenote");
    types_map->insert("onetmp", "application/onenote");
    types_map->insert("onetoc2", "application/onenote");
    types_map->insert("orc", "audio/csound");
    types_map->insert("orf", "image/x-olympus-orf");
    types_map->insert("otg", "application/vnd.oasis.opendocument.graphics-template");
    types_map->insert("oth", "application/vnd.oasis.opendocument.text-web");
    types_map->insert("otp", "application/vnd.oasis.opendocument.presentation-template");
    types_map->insert("ots", "application/vnd.oasis.opendocument.spreadsheet-template");
    types_map->insert("ott", "application/vnd.oasis.opendocument.text-template");
    types_map->insert("oxt", "application/vnd.openofficeorg.extension");
    types_map->insert("oza", "application/x-oz-application");
    types_map->insert("p", "text/x-pascal");
    types_map->insert("p12", "application/x-pkcs12");
    types_map->insert("p7c", "application/pkcs7-mime");
    types_map->insert("p7r", "application/x-pkcs7-certreqresp");
    types_map->insert("pac", "application/x-ns-proxy-autoconfig");
    types_map->insert("pas", "text/x-pascal");
    types_map->insert("pat", "image/x-coreldrawpattern");
    types_map->insert("patch", "text/x-diff");
    types_map->insert("pbm", "image/x-portable-bitmap");
    types_map->insert("pcap", "application/cap");
    types_map->insert("pcf", "application/x-font");
    types_map->insert("pcf.Z", "application/x-font");
    types_map->insert("pcx", "image/pcx");
    types_map->insert("pdb", "chemical/x-pdb");
    types_map->insert("pdf", "application/pdf");
    types_map->insert("pfa", "application/x-font");
    types_map->insert("pfb", "application/x-font");
    types_map->insert("pfx", "application/x-pkcs12");
    types_map->insert("pgm", "image/x-portable-graymap");
    types_map->insert("pgn", "application/x-chess-pgn");
    types_map->insert("pgp", "application/pgp-signature");
    types_map->insert("php", "application/x-httpd-php");
    types_map->insert("php3", "application/x-httpd-php3");
    types_map->insert("php3p", "application/x-httpd-php3-preprocessed");
    types_map->insert("php4", "application/x-httpd-php4");
    types_map->insert("php5", "application/x-httpd-php5");
    types_map->insert("phps", "application/x-httpd-php-source");
    types_map->insert("pht", "application/x-httpd-php");
    types_map->insert("phtml", "application/x-httpd-php");
    types_map->insert("pk", "application/x-tex-pk");
    types_map->insert("pl", "text/x-perl");
    types_map->insert("pls", "audio/x-scpls");
    types_map->insert("pm", "text/x-perl");
    types_map->insert("png", "image/png");
    types_map->insert("pnm", "image/x-portable-anymap");
    types_map->insert("pot", "text/plain");
    types_map->insert("potm", "application/vnd.ms-powerpoint.template.macroEnabled.12");
    types_map->insert("potx", "application/vnd.openxmlformats-officedocument.presentationml.template");
    types_map->insert("ppa", "application/vnd.ms-powerpoint");
    types_map->insert("ppam", "application/vnd.ms-powerpoint.addin.macroEnabled.12");
    types_map->insert("ppm", "image/x-portable-pixmap");
    types_map->insert("pps", "application/vnd.ms-powerpoint");
    types_map->insert("ppsm", "application/vnd.ms-powerpoint.slideshow.macroEnabled.12");
    types_map->insert("ppsx", "application/vnd.openxmlformats-officedocument.presentationml.slideshow");
    types_map->insert("ppt", "application/vnd.ms-powerpoint");
    types_map->insert("pptm", "application/vnd.ms-powerpoint.presentation.macroEnabled.12");
    types_map->insert("pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation");
    types_map->insert("prf", "application/pics-rules");
    types_map->insert("prt", "chemical/x-ncbi-asn1-ascii");
    types_map->insert("ps", "application/postscript");
    types_map->insert("psd", "image/x-photoshop");
    types_map->insert("pwz", "application/vnd.ms-powerpoint");
    types_map->insert("py", "text/x-python");
    types_map->insert("pyc", "application/x-python-code");
    types_map->insert("pyo", "application/x-python-code");
    types_map->insert("qgs", "application/x-qgis");
    types_map->insert("qt", "video/quicktime");
    types_map->insert("qtl", "application/x-quicktimeplayer");
    types_map->insert("ra", "audio/x-realaudio");
    types_map->insert("ram", "audio/x-pn-realaudio");
    types_map->insert("rar", "application/rar");
    types_map->insert("ras", "image/x-cmu-raster");
    types_map->insert("rb", "application/x-ruby");
    types_map->insert("rd", "chemical/x-mdl-rdfile");
    types_map->insert("rdf", "application/rdf+xml");
    types_map->insert("rdp", "application/x-rdp");
    types_map->insert("rgb", "image/x-rgb");
    types_map->insert("rhtml", "application/x-httpd-eruby");
    types_map->insert("rm", "audio/x-pn-realaudio");
    types_map->insert("roff", "application/x-troff");
    types_map->insert("ros", "chemical/x-rosdal");
    types_map->insert("rpm", "application/x-redhat-package-manager");
    types_map->insert("rss", "application/rss+xml");
    types_map->insert("rtf", "application/rtf");
    types_map->insert("rtx", "text/richtext");
    types_map->insert("rxn", "chemical/x-mdl-rxnfile");
    types_map->insert("scala", "text/x-scala");
    types_map->insert("sce", "application/x-scilab");
    types_map->insert("sci", "application/x-scilab");
    types_map->insert("sco", "audio/csound");
    types_map->insert("scr", "application/x-silverlight");
    types_map->insert("sct", "text/scriptlet");
    types_map->insert("sd", "chemical/x-mdl-sdfile");
    types_map->insert("sd2", "audio/x-sd2");
    types_map->insert("sda", "application/vnd.stardivision.draw");
    types_map->insert("sdc", "application/vnd.stardivision.calc");
    types_map->insert("sdd", "application/vnd.stardivision.impress");
    types_map->insert("sdf", "chemical/x-mdl-sdfile");
    types_map->insert("sdp", "application/vnd.stardivision.impress");
    types_map->insert("sds", "application/vnd.stardivision.chart");
    types_map->insert("sdw", "application/vnd.stardivision.writer");
    types_map->insert("ser", "application/java-serialized-object");
    types_map->insert("sfv", "text/x-sfv");
    types_map->insert("sgf", "application/x-go-sgf");
    types_map->insert("sgl", "application/vnd.stardivision.writer-global");
    types_map->insert("sgm", "text/x-sgml");
    types_map->insert("sgml", "text/x-sgml");
    types_map->insert("sh", "text/x-sh");
    types_map->insert("shar", "application/x-shar");
    types_map->insert("shp", "application/x-qgis");
    types_map->insert("shtml", "text/html");
    types_map->insert("shx", "application/x-qgis");
    types_map->insert("sid", "audio/prs.sid");
    types_map->insert("sik", "application/x-trash");
    types_map->insert("silo", "model/mesh");
    types_map->insert("sis", "application/vnd.symbian.install");
    types_map->insert("sisx", "x-epoc/x-sisx-app");
    types_map->insert("sit", "application/x-stuffit");
    types_map->insert("sitx", "application/x-stuffit");
    types_map->insert("skd", "application/x-koan");
    types_map->insert("skm", "application/x-koan");
    types_map->insert("skp", "application/x-koan");
    types_map->insert("skt", "application/x-koan");
    types_map->insert("sldm", "application/vnd.ms-powerpoint.slide.macroEnabled.12");
    types_map->insert("sldx", "application/vnd.openxmlformats-officedocument.presentationml.slide");
    types_map->insert("smf", "application/vnd.stardivision.math");
    types_map->insert("smi", "application/smil");
    types_map->insert("smil", "application/smil");
    types_map->insert("snd", "audio/basic");
    types_map->insert("so", "application/octet-stream");
    types_map->insert("spc", "chemical/x-galactic-spc");
    types_map->insert("spl", "application/x-futuresplash");
    types_map->insert("spx", "audio/ogg");
    types_map->insert("sql", "application/x-sql");
    types_map->insert("src", "application/x-wais-source");
    types_map->insert("stc", "application/vnd.sun.xml.calc.template");
    types_map->insert("std", "application/vnd.sun.xml.draw.template");
    types_map->insert("sti", "application/vnd.sun.xml.impress.template");
    types_map->insert("stl", "application/sla");
    types_map->insert("stw", "application/vnd.sun.xml.writer.template");
    types_map->insert("sty", "text/x-tex");
    types_map->insert("sv4cpio", "application/x-sv4cpio");
    types_map->insert("sv4crc", "application/x-sv4crc");
    types_map->insert("svg", "image/svg+xml");
    types_map->insert("svgz", "image/svg+xml");
    types_map->insert("sw", "chemical/x-swissprot");
    types_map->insert("swf", "application/x-shockwave-flash");
    types_map->insert("swfl", "application/x-shockwave-flash");
    types_map->insert("sxc", "application/vnd.sun.xml.calc");
    types_map->insert("sxd", "application/vnd.sun.xml.draw");
    types_map->insert("sxg", "application/vnd.sun.xml.writer.global");
    types_map->insert("sxi", "application/vnd.sun.xml.impress");
    types_map->insert("sxm", "application/vnd.sun.xml.math");
    types_map->insert("sxw", "application/vnd.sun.xml.writer");
    types_map->insert("t", "application/x-troff");
    types_map->insert("tar", "application/x-tar");
    types_map->insert("taz", "application/x-gtar-compressed");
    types_map->insert("tcl", "text/x-tcl");
    types_map->insert("tex", "text/x-tex");
    types_map->insert("texi", "application/x-texinfo");
    types_map->insert("texinfo", "application/x-texinfo");
    types_map->insert("text", "text/plain");
    types_map->insert("tgf", "chemical/x-mdl-tgf");
    types_map->insert("tgz", "application/x-gtar-compressed");
    types_map->insert("thmx", "application/vnd.ms-officetheme");
    types_map->insert("tif", "image/tiff");
    types_map->insert("tiff", "image/tiff");
    types_map->insert("tk", "text/x-tcl");
    types_map->insert("tm", "text/texmacs");
    types_map->insert("torrent", "application/x-bittorrent");
    types_map->insert("tr", "application/x-troff");
    types_map->insert("ts", "video/MP2T");
    types_map->insert("tsp", "application/dsptype");
    types_map->insert("tsv", "text/tab-separated-values");
    types_map->insert("txt", "text/plain");
    types_map->insert("udeb", "application/x-debian-package");
    types_map->insert("uls", "text/iuls");
    types_map->insert("ustar", "application/x-ustar");
    types_map->insert("val", "chemical/x-ncbi-asn1-binary");
    types_map->insert("vcd", "application/x-cdlink");
    types_map->insert("vcf", "text/x-vcard");
    types_map->insert("vcs", "text/x-vcalendar");
    types_map->insert("vmd", "chemical/x-vmd");
    types_map->insert("vms", "chemical/x-vamas-iso14976");
    types_map->insert("vor", "application/vnd.stardivision.writer");
    types_map->insert("vrm", "x-world/x-vrml");
    types_map->insert("vrml", "x-world/x-vrml");
    types_map->insert("vsd", "application/vnd.visio");
    types_map->insert("wad", "application/x-doom");
    types_map->insert("wav", "audio/x-wav");
    types_map->insert("wax", "audio/x-ms-wax");
    types_map->insert("wbmp", "image/vnd.wap.wbmp");
    types_map->insert("wbxml", "application/vnd.wap.wbxml");
    types_map->insert("webm", "video/webm");
    types_map->insert("wiz", "application/msword");
    types_map->insert("wk", "application/x-123");
    types_map->insert("wm", "video/x-ms-wm");
    types_map->insert("wma", "audio/x-ms-wma");
    types_map->insert("wmd", "application/x-ms-wmd");
    types_map->insert("wml", "text/vnd.wap.wml");
    types_map->insert("wmlc", "application/vnd.wap.wmlc");
    types_map->insert("wmls", "text/vnd.wap.wmlscript");
    types_map->insert("wmlsc", "application/vnd.wap.wmlscriptc");
    types_map->insert("wmv", "video/x-ms-wmv");
    types_map->insert("wmx", "video/x-ms-wmx");
    types_map->insert("wmz", "application/x-ms-wmz");
    types_map->insert("wp5", "application/vnd.wordperfect5.1");
    types_map->insert("wpd", "application/vnd.wordperfect");
    types_map->insert("wrl", "x-world/x-vrml");
    types_map->insert("wsc", "text/scriptlet");
    types_map->insert("wsdl", "application/xml");
    types_map->insert("wvx", "video/x-ms-wvx");
    types_map->insert("wz", "application/x-wingz");
    types_map->insert("x3d", "model/x3d+xml");
    types_map->insert("x3db", "model/x3d+binary");
    types_map->insert("x3dv", "model/x3d+vrml");
    types_map->insert("xbm", "image/x-xbitmap");
    types_map->insert("xcf", "application/x-xcf");
    types_map->insert("xht", "application/xhtml+xml");
    types_map->insert("xhtml", "application/xhtml+xml");
    types_map->insert("xlam", "application/vnd.ms-excel.addin.macroEnabled.12");
    types_map->insert("xlb", "application/vnd.ms-excel");
    types_map->insert("xls", "application/vnd.ms-excel");
    types_map->insert("xlsb", "application/vnd.ms-excel.sheet.binary.macroEnabled.12");
    types_map->insert("xlsm", "application/vnd.ms-excel.sheet.macroEnabled.12");
    types_map->insert("xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
    types_map->insert("xlt", "application/vnd.ms-excel");
    types_map->insert("xltm", "application/vnd.ms-excel.template.macroEnabled.12");
    types_map->insert("xltx", "application/vnd.openxmlformats-officedocument.spreadsheetml.template");
    types_map->insert("xml", "application/xml");
    types_map->insert("xpdl", "application/xml");
    types_map->insert("xpi", "application/x-xpinstall");
    types_map->insert("xpm", "image/x-xpixmap");
    types_map->insert("xsd", "application/xml");
    types_map->insert("xsl", "application/xml");
    types_map->insert("xspf", "application/xspf+xml");
    types_map->insert("xtel", "chemical/x-xtel");
    types_map->insert("xul", "application/vnd.mozilla.xul+xml");
    types_map->insert("xwd", "image/x-xwindowdump");
    types_map->insert("xyz", "chemical/x-xyz");
    types_map->insert("zip", "application/zip");
    types_map->insert("zmt", "chemical/x-mopac-input");
    types_map->insert("~", "application/x-trash");
}

QString splitPath(const QString& path, int *pos)
{
    if (path.isEmpty()) {
        return "";
    }

    QString p = QDir::fromNativeSeparators(path);
    while (p.endsWith('/') && p.size() > 1) {
        p = p.left(p.size() - 1);
    }
    if (p.size() == 1) {
        return p;
    }

    *pos = p.lastIndexOf("/");
    return p;
}


} // namespace

QString mimeTypeFromFileName(const QString& fileName)
{
    if (types_map == 0) {
        init();
    }

    QString suffix = QFileInfo(fileName).suffix().toLower();

    return types_map->value(suffix);
}

QString readableNameForFolder(bool readonly)
{
    return readonly ? QObject::tr("Readonly Folder") : QObject::tr("Folder");
}

QString readableNameForFile(const QString& fileName)
{
    QString mimetype = mimeTypeFromFileName(fileName);

    if (mimetype.isEmpty()) {
        return QObject::tr("Document");
    }

    if (mimetype == "application/pdf") {
        return QObject::tr("PDF Document");
    } else if (mimetype.startsWith("image")) {
        return QObject::tr("Image File");
    } else if (mimetype.startsWith("text")) {
        return QObject::tr("Text Document");
    } else if (mimetype.startsWith("audio")) {
        return QObject::tr("Audio File");
    } else if (mimetype.startsWith("video")) {
        return QObject::tr("Video File");
    } else if (mimetype.contains("msword") || mimetype.contains("ms-word")) {
        return QObject::tr("Word Document");
    } else if (mimetype.contains("mspowerpoint") || mimetype.contains("ms-powerpoint")) {
        return QObject::tr("PowerPoint Document");
    } else if (mimetype.contains("msexcel") || mimetype.contains("ms-excel")) {
        return QObject::tr("Excel Document");
    } else if (mimetype.contains("openxmlformats-officedocument")) {
        // see http://stackoverflow.com/questions/4212861/what-is-a-correct-mime-type-for-docx-pptx-etc
        if (mimetype.contains("wordprocessingml")) {
            return QObject::tr("Word Document");
        } else if (mimetype.contains("spreadsheetml")) {
            return QObject::tr("Excel Document");
        } else if (mimetype.contains("presentationml")) {
            return QObject::tr("PowerPoint Document");
        }
        // } else if (mimetype.contains("application")) {
        //     return "binary";
    }

    return QObject::tr("Document");
}

QString iconPrefixFromFileName(const QString& fileName)
{
    QString mimetype = mimeTypeFromFileName(fileName);

    if (mimetype.isEmpty()) {
        return "";
    }

    if (mimetype == "application/pdf") {
        return "pdf";
    } else if (mimetype.startsWith("image")) {
        return "image";
    } else if (mimetype.startsWith("text")) {
        return "text";
    } else if (mimetype.startsWith("audio")) {
        return "audio";
    } else if (mimetype.startsWith("video")) {
        return "video";
    } else if (mimetype.contains("msword") || mimetype.contains("ms-word")) {
        return "ms_word";
    } else if (mimetype.contains("mspowerpoint") || mimetype.contains("ms-powerpoint")) {
        return "ms_ppt";
    } else if (mimetype.contains("msexcel") || mimetype.contains("ms-excel")) {
        return "ms_excel";
    } else if (mimetype.contains("openxmlformats-officedocument")) {
        // see http://stackoverflow.com/questions/4212861/what-is-a-correct-mime-type-for-docx-pptx-etc
        if (mimetype.contains("wordprocessingml")) {
            return "ms_word";
        } else if (mimetype.contains("spreadsheetml")) {
            return "ms_excel";
        } else if (mimetype.contains("presentationml")) {
            return "ms_ppt";
        }
        // } else if (mimetype.contains("application")) {
        //     return "binary";
    } else if (mimetype.contains("7z") || mimetype.contains("rar") ||
               mimetype.contains("zip") || mimetype.startsWith("application/x-tar")) {
        return "zip";
    }

    return "";
}


QString getIconByFileName(const QString& fileName)
{
    QString icon = iconPrefixFromFileName(fileName);

    if (icon.isEmpty()) {
        icon = "unknown";
    }

    return QString(":/images/files/file_%1.png").arg(icon);
}

QString getIconByFolder()
{
    return QString(":/images/files/file_folder.png");
}

QString getIconByFileNameV2(const QString& fileName)
{
    QString icon = iconPrefixFromFileName(fileName);

    // Use doc icons for text files
    if (icon == "text") {
        icon = "ms_word";
    }

    return QString(":/images/files_v2/file_%1.png").arg(icon.isEmpty() ? "unknown" : icon);
}


QString pathJoin(const QString& a,
                 const QString& b)
{
    QStringList list(b);
    return pathJoin(a, list);
}


QString pathJoin(const QString& a,
                 const QString& b,
                 const QString& c)
{
    QStringList list;
    list << b << c;
    return pathJoin(a, list);
}

QString pathJoin(const QString& a,
                 const QString& b,
                 const QString& c,
                 const QString& d)
{
    QStringList list;
    list << b << c << d;
    return pathJoin(a, list);
}

QString pathJoin(const QString& a, const QStringList& rest)
{
    QString result = a;
    foreach (const QString& b, rest) {
        bool resultEndsWithSlash = result.endsWith("/");
        bool bStartWithSlash = b.startsWith("/");
        if (resultEndsWithSlash && bStartWithSlash) {
            result.append(b.right(b.size() - 1));
        } else if (resultEndsWithSlash || bStartWithSlash) {
            result.append(b);
        } else {
            result.append("/" + b);
        }
    }

    return result;
}

bool createDirIfNotExists(const QString& path)
{
    QDir parent_dir = getParentPath(path);
    return parent_dir.mkpath(getBaseName(path));
}

QString getParentPath(const QString& path)
{
    int pos;
    QString p = splitPath(path, &pos);
    if (p.size() <= 1) {
        return p;
    }

    if (pos == -1)
        return "";
    if (pos == 0)
        return "/";
    return p.left(pos);
}

QString getBaseName(const QString& path)
{
    int pos;
    QString p = splitPath(path, &pos);
    if (p.size() <= 1) {
        return p;
    }

    if (pos == -1) {
        return p;
    }
    return p.mid(pos + 1);
}

QString expandVars(const QString& origin)
{
#ifdef Q_OS_WIN32
    // expand environment strings
    QString retval;
    std::wstring worigin = origin.toStdWString();
    utils::WBufferArray expanded;
    expanded.resize(MAX_PATH);
    DWORD expanded_size = ExpandEnvironmentStringsW(&worigin[0], &expanded[0], MAX_PATH);
    if (expanded_size > 0) {
        expanded.resize(expanded_size);
        if (expanded_size > MAX_PATH)
            expanded_size = ExpandEnvironmentStringsW(&worigin[0], &expanded[0], expanded_size);
    }
    if (expanded_size > 0 && expanded_size == expanded.size()) {
        retval = QString::fromWCharArray(&expanded[0], expanded_size);
        // workaround with a bug
        retval = QString::fromUtf8(retval.toUtf8());
    } else {
        retval = origin;
    }
#else
    // TODO implement it
    QString retval = origin;
#endif
    return retval;
}

QString expandUser(const QString& origin)
{
    if (origin.startsWith("~")) {
        QChar seperator = QDir::separator();
        int pos = 1;
        while (pos < origin.size() && origin[pos] != seperator)
            ++pos;
        QString homepath;
        if (pos == 1)
            homepath = QDir::homePath();
        else
            homepath = QFileInfo(QDir::homePath()).dir().filePath(origin.mid(1, pos - 1));
        return pathJoin(homepath, origin.right(origin.size() - pos));
    }
    return origin;
}

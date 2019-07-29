#include <QCoreApplication>
#include <QDomDocument>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlRecord>
#include <QSqlQuery>
//#include <QHostAddress>

#include <stdio.h>

#include <QDebug>

static QSqlDatabase db;
static QDebug deb = qDebug();

struct host{
            QString macaddress	= "";
            QString hostname    = "";
            QVector<QString> ipaddress;
            QString interface	= "";
            QString type		= "";
            QString permanent 	= "";
            QString	published 	= "";
            QString expires		= "";
            int     id          = -1; //for hostFromDB;
            QString incomplete  = "";
          };

static QList<host> listHosts;

void addPropertyHostFormFile(host &h, QDomElement& domElement)
{
    if(domElement.tagName() == "hostname")
        h.hostname = domElement.text();
    else if(domElement.tagName() == "ip-address")
        h.ipaddress.append(domElement.text());
    else if(domElement.tagName() == "mac-address")
        h.macaddress = domElement.text();
    else if(domElement.tagName() == "interface")
        h.interface = domElement.text();
    else if(domElement.tagName() == "expires")
        h.expires = domElement.text();
    else if(domElement.tagName() == "type")
        h.type = domElement.text();
    else if(domElement.tagName() == "permanent")
        h.permanent = domElement.text();
    else if(domElement.tagName() == "published")
        h.published = domElement.text();
    else if(domElement.tagName() == "incomplete")
        h.incomplete = domElement.text();
}
bool findHostInTable(host &hostFromFile, host &hostFromDB)
{
    QSqlQuery query(db);

    QString strQuery = "SELECT * FROM hosts WHERE mac='" + hostFromFile.macaddress + "';";
    if(!query.exec(strQuery)){
        deb << __FUNCTION__ << __LINE__ << "Unable select from table hosts";
    }

    QSqlRecord rec = query.record();
    if(query.first())
    {
            hostFromDB.id           = query.value(rec.indexOf("id")).toInt();
            hostFromDB.macaddress   = query.value(rec.indexOf("mac")).toString();
            hostFromDB.hostname     = query.value(rec.indexOf("name")).toString();
            hostFromDB.interface    = query.value(rec.indexOf("iface")).toString();
            //        hostFromDB.type = query.value(rec.indexOf("mac")).toString();
            //        hostFromDB.permanent = query.value(rec.indexOf("mac")).toString();
            //        hostFromDB.published = query.value(rec.indexOf("mac")).toString();
            //        hostFromDB.expires = query.value(rec.indexOf("mac")).toString();

            QString strQuery = "SELECT * FROM history AS h1 WHERE mac_id=" + QString::number(hostFromDB.id) +
                               " AND created = (SELECT MAX(created) FROM history WHERE mac_id=" +
                               QString::number(hostFromDB.id) + ") ORDER BY h1.created;";

            if(!query.exec(strQuery))
                deb << __FUNCTION__ << __LINE__ << "Unable select from table history";

            QSqlRecord rec = query.record();
            while(query.next())
                hostFromDB.ipaddress.append(query.value(rec.indexOf("addr")).toString());

        return true;
    }
    return false;
}
bool toUpdate(host &hostFromFile, host &hostFromDB)
{
    if(hostFromDB.ipaddress.length() == hostFromFile.ipaddress.length())
        for(int i = 0; i < hostFromFile.ipaddress.length(); ++i){
            if(hostFromFile.ipaddress.at(i) != hostFromDB.ipaddress.at(i)){
                deb << hostFromFile.macaddress << " - " << "old " << hostFromFile.ipaddress.at(i) << "new " <<  hostFromDB.ipaddress.at(i);
                return true;
            }
        }

    return false;
}
void insertInTables(host hostFromFile)
{
    host hostFromDB;

    static int count = 0;

    deb <<  ++count;
    if(findHostInTable(hostFromFile, hostFromDB))
    {
        if(toUpdate(hostFromFile, hostFromDB))
        {
            QSqlQuery query(db);
            for(int i = 0; i < hostFromFile.ipaddress.length(); ++i)
            {
                query.prepare("INSERT INTO history (mac_id, addr)"
                              "VALUE (:mac_id, :addr);");
                query.bindValue(":mac_id", hostFromDB.id);
                query.bindValue(":addr", hostFromFile.ipaddress.at(i));

                if(!query.exec()){
                    deb << __FUNCTION__ << __LINE__ << "Unable insert in table history";
                }
            }
        }
    }
    else{
        QSqlQuery query(db);
        query.prepare("INSERT INTO hosts (mac, name, iface)"
                      "VALUE (:mac, :name, :iface);");
        query.bindValue(":mac", hostFromFile.macaddress);
        query.bindValue(":name", hostFromFile.hostname);
        query.bindValue(":iface", hostFromFile.interface);

        QString mac_id;
        if(query.exec())
            mac_id = query.lastInsertId().toString();
        else
            deb << __FUNCTION__ << __LINE__ << "Unable insert in table hosts";

        deb << "new " << hostFromFile.macaddress;
        for(int i = 0; i < hostFromFile.ipaddress.length(); ++i)
        {
            deb << " add ip - " << hostFromFile.ipaddress.at(i);
            query.prepare("INSERT INTO history (mac_id, addr)"
                          "VALUE (:mac_id, :addr);");
            query.bindValue(":mac_id", mac_id);
            query.bindValue(":addr", hostFromFile.ipaddress.at(i));

            if(!query.exec()){
                deb << __FUNCTION__ << __LINE__ << "Unable insert in table history";
            }
        }
    }
    deb << "\n";
}
bool checkArgs(const int &argc){
    if(argc < 2 || argc > 2){
        printf("Please input file path\n");
        exit(1);
    }
    return true;
}
// first arg - xmlInputFile
void pushBack(QList<host> &list, host &h)
{
    for(auto &i : list)
    {
        if(i.macaddress == h.macaddress){
            i.ipaddress.append(h.ipaddress.at(0));
            return;
        }
    }

    listHosts.append(h);
}
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if(checkArgs(argc)){
        QString fileName(argv[1]);

        db = QSqlDatabase::addDatabase("QMYSQL", "mydb");
        db.setHostName("192.168.0.1");
        db.setDatabaseName("hosts");
        db.setUserName("user");
        db.setPassword("password");
        bool ok = db.open();

        deb << "conected db = " << ok << "\n";

        QDomDocument domDoc;
        QFile        file(fileName);

        if(file.open(QIODevice::ReadOnly)) {
            QString errorStr; int errorLine; int errorColumn;

            if(domDoc.setContent(&file, false, &errorStr, &errorLine, &errorColumn)) {
                QDomElement rootElement = domDoc.documentElement();  // <arp version="1">
                QDomNode domNode = rootElement.firstChild();         // <arp-cache>

                //проход по всем тегам <arp-cache>
                while(!domNode.isNull()) {
                    if(domNode.isElement()) {
                        QDomElement domElement = domNode.toElement();
                        if(domElement.tagName() == "arp-cache")
                        {
                            QDomNode domNode = domElement.firstChild(); // первый элемент в <arp-cache>
                            // проход по всем элементам в теге <arp-cache>

                            host hostFile;
                            while(!domNode.isNull()) {
                                if(domNode.isElement()) {
                                    QDomElement domElement = domNode.toElement();
                                    addPropertyHostFormFile(hostFile, domElement);
                                 }
                                 domNode = domNode.nextSibling();
                            }
                            if(hostFile.incomplete != "true")
                                pushBack(listHosts, hostFile);
                        }
                    }
                    domNode = domNode.nextSibling();
                }
            }
            else
                deb << "Error: " << errorStr << " at line " << errorLine << " column " << errorColumn;

            file.close();
        }
        else
            fprintf(stderr, "error: can't open the file %s\n", qPrintable(file.fileName()));
    }

    int count = 0;
    for(auto &i : listHosts){
        printf("%5d: ", ++count);
        printf("id - %2d; ", i.id);
        printf("mac - %18s; ", i.macaddress.toStdString().c_str());
        printf("name - %5s; ", i.hostname.toStdString().c_str());
         printf("ip - ");
        for(auto &ip : i.ipaddress)
            printf("%16s ", ip.toStdString().c_str());
        printf("; iface -%5s\n", i.interface.toStdString().c_str());
    }

    for(auto host : listHosts)
        insertInTables(host);

    return 0;
}

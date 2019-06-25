//#include <QCoreApplication>
#include <QDomDocument>
#include <QFile>

#include <stdio.h>

#include <QDebug>

enum tags {
            hostname,
            ipaddress,
            macaddress,
            interface,
            expires,
            type
          };

bool checkArgs(const int &argc){
    if(argc < 2 || argc > 2){
        printf("Please input file path\n");
        exit(1);
    }
    return true;
}
// 1 arg - xmlInputFile
int main(int argc, char *argv[])
{
//    QCoreApplication a(argc, argv);

    if(checkArgs(argc)){
        QString fileName(argv[0]);

        QDomDocument domDoc;
    //    QFile        file("C:\\Qt\\myProg\\Hosts_module_xml-Sql\\mac.xml");
        QFile        file(fileName);

        if(file.open(QIODevice::ReadOnly)) {
            if(domDoc.setContent(&file)) {
                QDomElement rootElement = domDoc.documentElement();  // <arp version="1">
                QDomNode domNode = rootElement.firstChild();         // <arp-cache>

                //проход по <arp-cache>
                while(!domNode.isNull()) {
                    if(domNode.isElement()) {
                        QDomElement domElement = domNode.toElement();
                        if(domElement.tagName() == "arp-cache")
                        {
                            QDomNode domNode = domElement.firstChild(); // первый элемент в <arp-cache>
                            // проход по всем элементам в <arp-cache>
                             while(!domNode.isNull()) {
                                 if(domNode.isElement()) {
                                     QDomElement domElement = domNode.toElement();
                                     qDebug() << domElement.tagName() << " = " << domElement.text();
                                     if(domElement.tagName() == "hostname"){
                                         ;
                                     }
                                     else if(domElement.tagName() == "ip-address"){
                                         ;
                                     }
                                     else if(domElement.tagName() == "mac-address"){
                                         ;
                                     }
                                     else if(domElement.tagName() == "interface"){
                                         ;
                                     }
                                     else if(domElement.tagName() == "expires"){
                                         ;
                                     }
                                     else if(domElement.tagName() == "type"){
                                         ;
                                     }
                                     else if(domElement.tagName() == "permanent"){
                                         ;
                                     }
                                     else if(domElement.tagName() == "published"){
                                         ;
                                     }
                                 }
                                 domNode = domNode.nextSibling();
                             }
                        }
                    }
                    domNode = domNode.nextSibling();
                }
            }
            file.close();
        }
        else
            printf("error: can't open the file %s\n", qPrintable(file.fileName()));
    }

    return 0;
//    return a.exec();
}

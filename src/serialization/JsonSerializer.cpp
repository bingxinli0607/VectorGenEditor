#include "JsonSerializer.h"
#include "model/Document.h"
#include "model/Layer.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

QJsonObject JsonSerializer::toJsonObject(const Document &doc)
{
    return doc.toJson();
}

std::unique_ptr<Document> JsonSerializer::fromJsonObject(const QJsonObject &obj)
{
    return Document::fromJson(obj);
}

bool JsonSerializer::saveToFile(const Document &doc, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QJsonDocument jsonDoc(doc.toJson());
    file.write(jsonDoc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

std::unique_ptr<Document> JsonSerializer::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return nullptr;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (jsonDoc.isNull() || !jsonDoc.isObject())
        return nullptr;
    return Document::fromJson(jsonDoc.object());
}

#ifndef JSONSERIALIZER_H
#define JSONSERIALIZER_H

#include <QString>
#include <QJsonObject>
#include <memory>

class Document;

class JsonSerializer
{
public:
    /// Serialize Document to JSON string
    static QJsonObject toJsonObject(const Document &doc);

    /// Deserialize JSON object to a new Document
    static std::unique_ptr<Document> fromJsonObject(const QJsonObject &obj);

    /// Convenience: Save/Load via JSON file (Phase 7 will add full file I/O)
    static bool saveToFile(const Document &doc, const QString &filePath);
    static std::unique_ptr<Document> loadFromFile(const QString &filePath);
};

#endif // JSONSERIALIZER_H

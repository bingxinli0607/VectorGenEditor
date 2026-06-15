#ifndef SVGEXPORTER_H
#define SVGEXPORTER_H

#include <QString>

class Document;

class SvgExporter
{
public:
    /// Export the document as structured SVG string.
    /// Each shape becomes its own SVG element — does NOT use scene->render().
    static QString toSvg(const Document &doc);

    /// Convenience: write SVG to file
    static bool exportToFile(const Document &doc, const QString &filePath);
};

#endif // SVGEXPORTER_H

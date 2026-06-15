#ifndef SHAPEFACTORY_H
#define SHAPEFACTORY_H

#include <memory>
#include <QString>

class Shape;

class ShapeFactory
{
public:
    static std::unique_ptr<Shape> create(const QString &type);
    static QStringList registeredTypes();
};

#endif // SHAPEFACTORY_H

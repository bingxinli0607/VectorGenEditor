#ifndef LAYER_H
#define LAYER_H

#include <QString>
#include <QJsonObject>
#include <vector>
#include <memory>

class Shape;

class Layer
{
public:
    explicit Layer(const QString &name = "Layer 1");
    ~Layer();

    QString name() const;
    void setName(const QString &name);

    bool visible() const;
    void setVisible(bool v);

    // --- Shape management (ordered, for z-order) ---
    const std::vector<std::unique_ptr<Shape>> &shapes() const;
    std::vector<std::unique_ptr<Shape>> &shapes();

    void addShape(std::unique_ptr<Shape> shape);
    std::unique_ptr<Shape> takeShape(const QString &shapeId);
    Shape *findShape(const QString &shapeId) const;

    // z-order
    void moveShapeUp(const QString &shapeId);
    void moveShapeDown(const QString &shapeId);
    void moveShapeToTop(const QString &shapeId);
    void moveShapeToBottom(const QString &shapeId);

    QJsonObject toJson() const;
    static std::unique_ptr<Layer> fromJson(const QJsonObject &obj);

private:
    QString m_name;
    bool    m_visible = true;
    std::vector<std::unique_ptr<Shape>> m_shapes;
};

#endif // LAYER_H

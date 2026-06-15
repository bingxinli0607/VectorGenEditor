#ifndef SHAPE_H
#define SHAPE_H

#include <QString>
#include <QJsonObject>
#include <QRectF>
#include <QPointF>
#include <memory>

// --- Data structures ---

struct Transform {
    double x = 0.0;
    double y = 0.0;
    double width = 100.0;
    double height = 100.0;
    double rotation = 0.0; // degrees

    QJsonObject toJson() const;
    static Transform fromJson(const QJsonObject &obj);
    QRectF boundingRect() const;
};

struct Style {
    QString fillColor   = "#4A90D9";
    QString strokeColor = "#333333";
    double strokeWidth  = 2.0;
    bool visible        = true;
    /// "solid" or "dash" — reserved for stroke line style
    QString lineStyle   = "solid";

    QJsonObject toJson() const;
    static Style fromJson(const QJsonObject &obj);
};

// --- Abstract Shape ---

class Shape
{
public:
    Shape();
    virtual ~Shape();

    // --- Identity ---
    QString id() const;
    void setId(const QString &id);
    virtual QString shapeType() const = 0;

    // --- Transform ---
    Transform transform() const;
    void setTransform(const Transform &t);
    QRectF boundingRect() const;

    // --- Style ---
    Style style() const;
    void setStyle(const Style &s);

    // --- Polymorphic interface ---
    virtual std::unique_ptr<Shape> clone() const = 0;
    virtual QJsonObject toJson() const;
    virtual void fromJson(const QJsonObject &obj);
    virtual QVector<QPointF> vertices() const { return {}; }

    /// Move shape in scene space. Vertex shapes keep local vertices unchanged.
    virtual void translateBy(double dx, double dy);

    /// Scale local vertices when geometry is resized (no-op for rect-like shapes).
    virtual void scaleLocalGeometry(const QRectF &oldBounds, const QRectF &newBounds);

    /// Replace local vertices during interactive resize preview.
    virtual void replaceLocalVertices(const QVector<QPointF> &local);

protected:
    // Subclasses populate shape-specific fields
    virtual QJsonObject typeSpecificToJson() const { return {}; }
    virtual void typeSpecificFromJson(const QJsonObject &) {}

private:
    QString   m_id;
    Transform m_transform;
    Style     m_style;
};

// --- Helpers ---
QString generateShapeId();

#endif // SHAPE_H

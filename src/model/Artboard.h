#ifndef ARTBOARD_H
#define ARTBOARD_H

#include <QRectF>

#include "Workspace.h"

/// Shared artboard (white page) rect in scene coordinates.
namespace Artboard {

inline QRectF rect()
{
    return Workspace::rect();
}

} // namespace Artboard

#endif // ARTBOARD_H

#define PI (3.14159f)
#define TO_RADIANS(angle) (angle * PI / 180.f)

#include <cmath>

typedef struct Point2DF {
	float x, y;
} Point2DF;

inline void RotatePoint2D(Point2DF& point, float angle, const Point2DF& origin = { 0, 0 }, const bool radians = false) {
	if (!radians) {
		angle = TO_RADIANS(angle);
	}

	float pointXLast = point.x;
	point.x = cos(angle) * (point.x - origin.x) - sin(angle) * (point.y - origin.y) + origin.x;
	point.y = sin(angle) * (pointXLast - origin.x) + cos(angle) * (point.y - origin.y) + origin.y;
}

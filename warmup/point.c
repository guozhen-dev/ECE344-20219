#include <assert.h>
#include "common.h"
#include "point.h"
#include <math.h>

void
point_translate(struct point *p, double x, double y)
{
	p->x += x;
	p->y += y;
}

double
point_distance(const struct point *p1, const struct point *p2)
{
	return sqrt((p1->x - p2->x) * (p1->x - p2->x) + (p1->y - p2->y) * (p1->y - p2->y));
}

int
point_compare(const struct point *p1, const struct point *p2)
{
	struct point * pt = malloc(sizeof(struct point));
	point_set(pt, 0, 0);
	double d1 = point_distance(p1,pt);
	double d2 = point_distance(p2,pt);
	if (d1 == d2) return 0;
	else if (d1 > d2) return 1;
	else return -1;
	// return 0;
}

#include "Haversine.h"
#include <cmath>
double haversineKm(double a, double o, double b, double p) { constexpr double pi=3.141592653589793, r=6371.0; const double dlat=(b-a)*pi/180, dlon=(p-o)*pi/180; const double x=std::sin(dlat/2)*std::sin(dlat/2)+std::cos(a*pi/180)*std::cos(b*pi/180)*std::sin(dlon/2)*std::sin(dlon/2); return 2*r*std::asin(std::sqrt(x)); }

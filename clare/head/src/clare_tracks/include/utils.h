double mapf(double x, double in_min, double in_max, double out_min, double out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

double clip(const double v, const double mn, const double mx) {
    if(v < mn) return mn;
    if(v > mx) return mx;
    return v;
}
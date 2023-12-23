#pragma once
#include <vector>

// linear regression on constant intervals
class LinearRegression
{
  public:
    double slope;
    double intercept;

    explicit LinearRegression(const std::vector<double>& v)
    {
        double sum_x = 0.0;
        double sum_y = 0.0;
        double sum_xy = 0.0;
        double sum_x_squared = 0.0;

        for (std::size_t i = 0; i < v.size(); ++i)
        {
            sum_x += i;
            sum_y += v[i];
            sum_xy += i * v[i];
            sum_x_squared += i * i;
        }

        double n = static_cast<double>(v.size());
        double mean_x = sum_x / n;
        double mean_y = sum_y / n;

        slope = (sum_xy - n * mean_x * mean_y) /
                (sum_x_squared - n * mean_x * mean_x);
        intercept = mean_y - slope * mean_x;
    }
};

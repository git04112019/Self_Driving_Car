#ifndef VEHICLE_H
#define VEHICLE_H
#include <iostream>
#include <random>
#include <sstream>
#include <fstream>
#include <math.h>
#include <vector>
#include <map>
#include <string>
#include <iterator>

using namespace std;

class Vehicle {

public:

  int L = 1;

  double pos_x, pos_y, angle, pos_s, pos_d;

  vector<double> next_x_vals, next_y_vals;

  int lane;

  int speed;

  int preferred_buffer = 6; // impacts "keep lane" behavior.

  int s;

  int v;

  int a;

  int target_speed;

  int lanes_available;

  int max_acceleration;

  int goal_lane;

  int goal_s;

  string state;

  /**
  * Constructor
  */
  Vehicle();

  /**
  * Destructor
  */
  virtual ~Vehicle();


  void UpdatePosition(double pos_x, double pos_y, double pos_s, double pos_d, double angle);

  void UpdateTrajectory(vector<double> next_x_vals, vector<double> next_y_vals, int next_map_point,
                                vector<double> map_waypoints_x, vector<double> map_waypoints_y, vector<double> map_waypoints_s);
};

#endif

#include <fstream>
#include <math.h>
#include <uWS/uWS.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include "Eigen-3.3/Eigen/Core"
#include "Eigen-3.3/Eigen/QR"
#include "json.hpp"
#include "spline.h"
#include "vehicle.h"
#include "vehicle.cpp"
#include "transforms.h"
#include "road.h"
#include "road.cpp"

using namespace std;

// for convenience
using json = nlohmann::json;


// Checks if the SocketIO event has JSON data.
// If there is data the JSON object in string format will be returned,
// else the empty string "" will be returned.
string hasData(string s) {
  auto found_null = s.find("null");
  auto b1 = s.find_first_of("[");
  auto b2 = s.find_first_of("}");
  if (found_null != string::npos) {
    return "";
  } else if (b1 != string::npos && b2 != string::npos) {
    return s.substr(b1, b2 - b1 + 2);
  }
  return "";
}


int main() {
  uWS::Hub h;

  // Define the landscape
  string map_file_ = "../data/highway_map.csv";
  Road road = Road(map_file_);

  // Define self as vehicle
  Vehicle ego = Vehicle(0); // ego has id = 0


  h.onMessage([&ego, &road](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length,
                     uWS::OpCode opCode) {
    // "42" at the start of the message means there's a websocket message event.
    // The 4 signifies a websocket message
    // The 2 signifies a websocket event
    //auto sdata = string(data).substr(0, length);
    //cout << sdata << endl;


    if (length && length > 2 && data[0] == '4' && data[1] == '2') {

      auto s = hasData(data);

      if (s != "") {
        auto j = json::parse(s);
        
        string event = j[0].get<string>();
        
        if (event == "telemetry") {
          // j[1] is the data JSON object
          
        	// Main car's localization Data
          	double car_x = j[1]["x"];
          	double car_y = j[1]["y"];
          	double car_s = j[1]["s"];
          	double car_d = j[1]["d"];
          	double car_yaw = j[1]["yaw"];
          	double car_speed = j[1]["speed"];

          	// Previous path data given to the Planner
          	auto previous_path_x = j[1]["previous_path_x"];
          	auto previous_path_y = j[1]["previous_path_y"];
          	// Previous path's end s and d values 
          	double end_path_s = j[1]["end_path_s"];
          	double end_path_d = j[1]["end_path_d"];

          	// Sensor Fusion Data, a list of all other cars on the same side of the road.
          	auto sensor_fusion = j[1]["sensor_fusion"];

                map<int , vector<double>> fusion;
                for (int i=0; i < sensor_fusion.size(); i++){
                        int vid = sensor_fusion[i][0]; // 0 is reserved for ego
                        vid++;
                        vector<double> v = sensor_fusion[i]; // temporary variable
                        vector<double> state(v.begin() + 1, v.end());
                        fusion[vid] = state;

                }

 //               Road.UpdateCars(fusion);



                json msgJson;

                vector<double> next_x_vals;
                vector<double> next_y_vals;


          	// TODO: define a path made up of (x,y) points that the car will visit sequentially every .02 seconds

                double pos_x;
                double pos_y;
                double angle;
                double pos_s;
                double pos_d;
                double des_d; // Desired d of the car, relates to lane

                int path_size = previous_path_x.size();

                for(int i = 0; i < path_size; i++)
                {
                    next_x_vals.push_back(previous_path_x[i]);
                    next_y_vals.push_back(previous_path_y[i]);
                }


                if(path_size == 0)
                {
                    pos_x = car_x;
                    pos_y = car_y;
                    angle = deg2rad(car_yaw);
                }
                else
                {
                    pos_x = previous_path_x[path_size-1];
                    pos_y = previous_path_y[path_size-1];

                    double pos_x2 = previous_path_x[path_size-2];
                    double pos_y2 = previous_path_y[path_size-2];
                    angle = atan2(pos_y-pos_y2,pos_x-pos_x2);
                }

                const int speed = 50; // mph

                // Update Car Position
                ego.UpdatePosition(pos_x,pos_y,angle, road.map_waypoints_x, road.map_waypoints_y);

                // calculate next mappoints for ego
                double dist_inc = 0.9*speed*1.609/3.6*0.02;

                int next_map_point;
                next_map_point = NextWaypoint(ego.pos_x, ego.pos_y, ego.angle,
                                              road.map_waypoints_x, road.map_waypoints_y);
                ego.UpdateTrajectory(next_x_vals, next_y_vals, next_map_point,
                                     road.map_waypoints_x, road.map_waypoints_y,road.map_waypoints_s);






                msgJson["next_x"] = ego.next_x_vals;
                msgJson["next_y"] = ego.next_y_vals;


                auto msg = "42[\"control\","+ msgJson.dump()+"]";

          	//this_thread::sleep_for(chrono::milliseconds(1000));
                ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
          
        }
      } else {
        // Manual driving
        std::string msg = "42[\"manual\",{}]";
        ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
      }
    }
  });

  // We don't need this since we're not using HTTP but if it's removed the
  // program
  // doesn't compile :-(
  h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data,
                     size_t, size_t) {
    const std::string s = "<h1>Hello world!</h1>";
    if (req.getUrl().valueLength == 1) {
      res->end(s.data(), s.length());
    } else {
      // i guess this should be done more gracefully?
      res->end(nullptr, 0);
    }
  });

  h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
    std::cout << "Connected!!!" << std::endl;
  });

  h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code,
                         char *message, size_t length) {
    ws.close();
    std::cout << "Disconnected" << std::endl;
  });

  int port = 4567;
  if (h.listen(port)) {
    std::cout << "Listening to port " << port << std::endl;
  } else {
    std::cerr << "Failed to listen to port" << std::endl;
    return -1;
  }
  h.run();
}

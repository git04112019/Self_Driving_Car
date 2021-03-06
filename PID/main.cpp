#include <uWS/uWS.h>
#include <iostream>
#include "json.hpp"
#include "PID.h"
#include <math.h>
#include <fstream>

// for convenience
using json = nlohmann::json;

// For converting back and forth between radians and degrees.
constexpr double pi() { return M_PI; }
double deg2rad(double x) { return x * pi() / 180; }
double rad2deg(double x) { return x * 180 / pi(); }

// Checks if the SocketIO event has JSON data.
// If there is data the JSON object in string format will be returned,
// else the empty string "" will be returned.
std::string hasData(std::string s) {
  auto found_null = s.find("null");
  auto b1 = s.find_first_of("[");
  auto b2 = s.find_last_of("]");
  if (found_null != std::string::npos) {
    return "";
  }
  else if (b1 != std::string::npos && b2 != std::string::npos) {
    return s.substr(b1, b2 - b1 + 1);
  }
  return "";
}

std::ofstream myfile;


int main()
{
  uWS::Hub h;

  PID pid;
  // TODO: Initialize the pid variable.
  pid.Init( 0.2, 0.004, 3);

  // define the file

  myfile.open ("example.txt");


  h.onMessage([&pid](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode) {
    // "42" at the start of the message means there's a websocket message event.
    // The 4 signifies a websocket message
    // The 2 signifies a websocket event

    // Variables for twiddle
    std::vector<double> p({pid.Kp, pid.Ki, pid.Kd});
    std::vector<double> dp({.1,.01,.1});
    double err, best_err;
    double dp_sum = dp[0]+dp[1]+dp[2];

    if (length && length > 2 && data[0] == '4' && data[1] == '2')
    {
      auto s = hasData(std::string(data));
      if (s != "") {
        auto j = json::parse(s);
        std::string event = j[0].get<std::string>();
        if (event == "telemetry") {
          // j[1] is the data JSON object
          double cte = std::stod(j[1]["cte"].get<std::string>());
          double speed = std::stod(j[1]["speed"].get<std::string>());
          double angle = std::stod(j[1]["steering_angle"].get<std::string>());
          double steer_value;
          double throttle;
          int it = 0;

          /*
          * TODO: Calcuate steering value here, remember the steering value is
          * [-1, 1].
          * NOTE: Feel free to play around with the throttle and speed. Maybe use
          * another PID controller to control the speed!
          */

          myfile<<"branch,err, Kp, Kd, Ki, p0,p1,p2,dp0,dp1,dp2"<<std::endl;

          // Initial run
          pid.UpdateError(cte);
          throttle = 0.3;
          steer_value = -pid.Kp*pid.p_error - pid.Kd*pid.d_error - pid.Ki*pid.i_error;

          json msgJson;
          msgJson["steering_angle"] = steer_value;
          msgJson["throttle"] = throttle;
          auto msg = "42[\"steer\"," + msgJson.dump() + "]";
          std::cout << msg << std::endl;
          ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);

          it = 0;

          // Adding twiddle
          while(dp_sum > 0.001){
              for(int i=0; i<3; i++){
                  p[i] += dp[i];
                  err = cte;
                  pid.Kp = p[0];
                  pid.Ki = p[1];
                  pid.Kd = p[2];
                  steer_value = -pid.Kp*pid.p_error - pid.Kd*pid.d_error - pid.Ki*pid.i_error;
                  pid.UpdateError(err);



                  if (err < best_err){
                      best_err = err;
                      dp[i] *= 1.1;
                      myfile<<"0,"<<err<<","<<pid.Kp <<","<< pid.Kd << ","<< pid.Ki<<"," \
                           <<p[0]<<","<<p[1]<<","<<p[2]<<"," \
                           <<dp[0]<<","<<dp[1]<<","<<dp[2]<<std::endl;

                  }
                   else{
                      p[i] -= 2*dp[i];
                      pid.Kp = p[0];
                      pid.Ki = p[1];
                      pid.Kd = p[2];
                      steer_value = -pid.Kp*pid.p_error - pid.Kd*pid.d_error - pid.Ki*pid.i_error;
                      pid.UpdateError(err);

                      if (err < best_err){
                          best_err = err;
                          dp[i] *= 1.1;
                          myfile<<"1,"<<err<<","<<pid.Kp <<","<< pid.Kd << ","<< pid.Ki<<"," \
                          <<p[0]<<","<<p[1]<<","<<p[2]<<"," \
                               <<dp[0]<<","<<dp[1]<<","<<dp[2]<<std::endl;
                      }
                      else{
                          p[i] += dp[i];
                          dp[i] *= 0.9;
                          myfile<<"2,"<<err<<","<<pid.Kp <<","<< pid.Kd << ","<< pid.Ki<<"," \
                          <<p[0]<<","<<p[1]<<","<<p[2]<<"," \
                               <<dp[0]<<","<<dp[1]<<","<<dp[2]<<std::endl;
                      }
                      }
                  }

                  it += 1;
              }
              myfile.close();
              dp_sum = dp[0]+dp[1]+dp[2];
              std::cout<<"dpSum : " << dp_sum << std::endl;

          }

          /*
def twiddle(tol=0.2):
    p = [0, 0, 0]
    dp = [1, 1, 1]
    robot = make_robot()
    x_trajectory, y_trajectory, best_err = run(robot, p)

    it = 0
    while sum(dp) > tol:
        print("Iteration {}, best error = {}".format(it, best_err))
        for i in range(len(p)):
            p[i] += dp[i]
            robot = make_robot()
            x_trajectory, y_trajectory, err = run(robot, p)

            if err < best_err:
                best_err = err
                dp[i] *= 1.1
            else:
                p[i] -= 2 * dp[i]
                robot = make_robot()
                x_trajectory, y_trajectory, err = run(robot, p)

                if err < best_err:
                    best_err = err
                    dp[i] *= 1.1
                else:
                    p[i] += dp[i]
                    dp[i] *= 0.9
        it += 1
    return p
          */

          std::cout<<"[Kp, Kd, Ki]: "<<pid.Kp <<" , "<< pid.Kd << " , "<< pid.Ki << std::endl;
          std::cout<<"[P, D, I]: "<<pid.p_error <<" , "<< pid.d_error << " , "<< pid.i_error << std::endl;

//          throttle = 0.3;
          //throttle = std::max(0.3, -0.1/0.05 * abs(steer_value) + 0.55);



//          std::cout << "CTE: " << cte << " Steering Value: " << steer_value << std::endl;
//          std::cout << "Total error : "<< sqrt(pid.TotalError()) << std::endl;
//          json msgJson;
//          msgJson["steering_angle"] = steer_value;
//          msgJson["throttle"] = throttle;
//          auto msg = "42[\"steer\"," + msgJson.dump() + "]";
//          std::cout << msg << std::endl;
//          ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);

        }
      } else {
        // Manual driving
        std::string msg = "42[\"manual\",{}]";
        ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
      }

  });

  // We don't need this since we're not using HTTP but if it's removed the program
  // doesn't compile :-(
  h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t, size_t) {
    const std::string s = "<h1>Hello world!</h1>";
    if (req.getUrl().valueLength == 1)
    {
      res->end(s.data(), s.length());
    }
    else
    {
      // i guess this should be done more gracefully?
      res->end(nullptr, 0);
    }
  });

  h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
    std::cout << "Connected!!!" << std::endl;
  });

  h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code, char *message, size_t length) {
    ws.close();
    std::cout << "Disconnected" << std::endl;
  });

  int port = 4567;
  if (h.listen(port))
  {
    std::cout << "Listening to port " << port << std::endl;
  }
  else
  {
    std::cerr << "Failed to listen to port" << std::endl;
    return -1;
  }
  h.run();
}

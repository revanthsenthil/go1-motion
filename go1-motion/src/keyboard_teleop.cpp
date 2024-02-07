#include "ros/ros.h"
#include <geometry_msgs/Twist.h>
#include <signal.h>
#include <termios.h>
#include <stdio.h>

#define KEYCODE_W 0x77
#define KEYCODE_A 0x61
#define KEYCODE_S 0x73
#define KEYCODE_D 0x64
#define KEYCODE_Q 0x71
#define KEYCODE_E 0x65
#define KEYCODE_SPACE 0x20  // E-stop

class KeyboardTeleop
{
public:
    KeyboardTeleop();

private:
    void keyLoop();
    ros::NodeHandle nh_;
    double linear_, angular_, l_scale_, a_scale_;
    ros::Publisher twist_pub_;
};

KeyboardTeleop::KeyboardTeleop():
    linear_(0),
    angular_(0),
    l_scale_(2.0),
    a_scale_(2.0)
{
    nh_.param("scale_angular", a_scale_, a_scale_);
    nh_.param("scale_linear", l_scale_, l_scale_);

    twist_pub_ = nh_.advertise<geometry_msgs::Twist>("/cmd_vel", 1);
}

int kfd = 0;
struct termios cooked, raw;

void quit(int sig)
{
    tcsetattr(kfd, TCSANOW, &cooked);
    ros::shutdown();
    exit(0);
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "keyboard_teleop");
    KeyboardTeleop keyboard_teleop;

    signal(SIGINT, quit);

    keyboard_teleop.keyLoop();

    return(0);
}

void KeyboardTeleop::keyLoop()
{
    char c;
    bool dirty = false;

    // get the console in raw mode                                                              
    tcgetattr(kfd, &cooked);
    memcpy(&raw, &cooked, sizeof(struct termios));
    raw.c_lflag &=~ (ICANON | ECHO);
    // Setting a new line, then end of file                         
    raw.c_cc[VEOL] = 1;
    raw.c_cc[VEOF] = 2;
    tcsetattr(kfd, TCSANOW, &raw);

    puts("Reading from keyboard");
    puts("---------------------------");
    puts("Use WASD keys to control the robot");
    puts("Use Q/E to rotate");
    puts("Press SPACE for emergency stop");

    while (ros::ok())
    {
        // get the next event from the keyboard  
        if(read(kfd, &c, 1) < 0)
        {
            perror("read():");
            exit(-1);
        }

        linear_ = angular_ = 0;
        ROS_DEBUG("value: 0x%02X\n", c);
  
        switch(c)
        {
            case KEYCODE_W:
                linear_ = 1.0 * l_scale_;
                dirty = true;
                break;
            case KEYCODE_S:
                linear_ = -1.0 * l_scale_;
                dirty = true;
                break;
            case KEYCODE_A:
                angular_ = 1.0 * a_scale_;
                dirty = true;
                break;
            case KEYCODE_D:
                angular_ = -1.0 * a_scale_;
                dirty = true;
                break;
            case KEYCODE_Q:
                angular_ = 1.0 * a_scale_; // Rotate counter-clockwise
                dirty = true;
                break;
            case KEYCODE_E:
                angular_ = -1.0 * a_scale_; // Rotate clockwise
                dirty = true;
                break;
            case KEYCODE_SPACE:
                ROS_INFO("Emergency stop activated!");
                linear_ = 0;
                angular_ = 0;
                dirty = true;
                break;
        }

        geometry_msgs::Twist twist;
        twist.linear.x = linear_;
        twist.angular.z = angular_;
        if(dirty == true)
        {
            twist_pub_.publish(twist);    
            dirty = false;
        }
    }
    return;
}

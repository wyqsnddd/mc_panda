#pragma once

#include <mc_panda/devices/api.h>

#include <mc_rbdyn/Device.h>

#include <mc_rtc/log/Logger.h>

#include <franka/model.h>
#include <franka/robot.h>

#include <condition_variable>
#include <queue>
#include <thread>

namespace mc_panda
{

/** This device provides:
 * - access to the commands in franka::Robot
 * - access to the associated franka::RobotState
 *
 * Commands are implemented in an asynchronous fashion in order not to
 * interfere with the control loop.
 *
 * If the robot is not connected (e.g. in simulation) the state has default
 * values and the commands are no-op. The connection status can be checked.
 *
 */
struct MC_PANDA_DEVICES_DLLAPI Robot : public mc_rbdyn::Device
{
  static constexpr auto name = "Robot";

  /** Get the device associated to the provided robot
   *
   * \returns nullptr if the device does not exist in this robot
   */
  static Robot * get(mc_rbdyn::Robot & robot);

  /** Constructor
   *
   * @param name Name of the Robot
   *
   */
  inline Robot() : mc_rbdyn::Device(name)
  {
    type_ = "Robot";
  }

  ~Robot() override;

  inline bool connected() const noexcept
  {
    return robot_ != nullptr;
  }

  /** Set the robot instance and start a command thread */
  void connect(franka::Robot * robot);

  /** Interrupt the connection to the robot */
  void disconnect();

  /** Set the current state */
  inline void state(const franka::RobotState & s) noexcept
  {
    state_ = s;
  }

  /** Returns the current state */
  inline const franka::RobotState & state() const noexcept
  {
    return state_;
  }

  /** Log the state information that is not passed to mc_rtc */
  void addToLogger(mc_rtc::Logger & logger, const std::string & prefix);

  /** Remove from the logger */
  void removeFromLogger(mc_rtc::Logger & logger, const std::string & prefix);

  mc_rbdyn::DevicePtr clone() const override;

  /** Set dynamics parameter of a payload, see franka::Robot documentation for details */
  void setLoad(double load_mass, const std::array<double, 3> & F_x_Cload, const std::array<double, 9> & load_inertia);

  /** Changes the collision behavior, see franka::Robot documentation for details */
  void setCollisionBehavior(const std::array<double, 7> & lower_torque_thresholds_acceleration,
                            const std::array<double, 7> & upper_torque_thresholds_acceleration,
                            const std::array<double, 7> & lower_torque_thresholds_nominal,
                            const std::array<double, 7> & upper_torque_thresholds_nominal,
                            const std::array<double, 6> & lower_force_thresholds_acceleration,
                            const std::array<double, 6> & upper_force_thresholds_acceleration,
                            const std::array<double, 6> & lower_force_thresholds_nominal,
                            const std::array<double, 6> & upper_force_thresholds_nominal);

  /** Changes the collision behavior, see franka::Robot documentation for details */
  void setCollisionBehavior(const std::array<double, 7> & lower_torque_thresholds,
                            const std::array<double, 7> & upper_torque_thresholds,
                            const std::array<double, 6> & lower_force_thresholds,
                            const std::array<double, 6> & upper_force_thresholds);

  /** Sets the impedance for each joint in the internal controller, see franka::Robot documentation for details */
  void setJointImpedance(const std::array<double, 7> & K_theta);

  /** Sets the cartesian impedance in the internal controller, see franka::Robot documentation for details */
  void setCartesianImpedance(const std::array<double, 6> & K_x);

  /** Stop all currently running motions, see franka::Robot documentation for details */
  void stop();

  /** Loads the  model library from the robot */
  inline franka::Model model() const noexcept
  {
    return robot_->loadModel();
  }

private:
  std::thread commandThread_;
  std::mutex commandMutex_;
  std::condition_variable commandCv_;
  struct Command
  {
    template<typename Callable>
    Command(const char * name, Callable && cb) : name(name), command(cb)
    {
    }
    /** For logging purpose */
    std::string name;
    /** Command callback */
    std::function<void()> command;
  };
  std::queue<Command> commands_;

  std::mutex robotMutex_;
  franka::Robot * robot_ = nullptr;
  franka::RobotState state_;
};

} // namespace mc_panda

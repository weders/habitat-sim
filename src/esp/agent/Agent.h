// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <map>
#include <set>
#include <string>

#include "esp/core/esp.h"
#include "esp/scene/ObjectControls.h"
#include "esp/scene/SceneNode.h"
#include "esp/sensor/Sensor.h"

namespace esp {
namespace agent {

// Represents the physical state of an agent
struct AgentState {
  vec3f position;
  // TODO: rotation below exposes quaternion x,y,z,w as vec4f for pybind11
  // interop, replace with quatf when we have custom pybind11 type conversion
  // for quaternions
  vec4f rotation;
  vec3f velocity;
  vec3f angularVelocity;
  vec3f force;
  vec3f torque;
  ESP_SMART_POINTERS(AgentState);
};

typedef std::map<std::string, float> ActuationMap;

// Specifies an action (i.e. name -> agent actuation).
struct ActionSpec {
  explicit ActionSpec(const std::string& _name, const ActuationMap& _actuation)
      : name(_name), actuation(_actuation) {}
  // action name
  std::string name;
  // linear, angular forces, joint torques, sensor actuation
  ActuationMap actuation;
  ESP_SMART_POINTERS(ActionSpec);
};
bool operator==(const ActionSpec& a, const ActionSpec& b);
bool operator!=(const ActionSpec& a, const ActionSpec& b);

// Represents a set of possible agent actions.
typedef std::map<std::string, ActionSpec::ptr> ActionSpace;

// Represents a configuration for an embodied Agent
struct AgentConfiguration {
  float height = 1.5;
  float radius = 0.1;
  float mass = 32.0;
  float linearAcceleration = 20.0;
  float angularAcceleration = 4 * 3.14;
  float linearFriction = 0.5;
  float angularFriction = 1.0;
  float coefficientOfRestitution = 0.0;

  std::vector<sensor::SensorSpec::ptr> sensorSpecifications = {
      sensor::SensorSpec::create()  // default SensorSpec
  };
  ActionSpace actionSpace = {  // default ActionSpace
      {"moveForward",
       ActionSpec::create("moveForward", ActuationMap{{"amount", 0.25f}})},
      {"lookLeft",
       ActionSpec::create("lookLeft", ActuationMap{{"amount", 10.0f}})},
      {"lookRight",
       ActionSpec::create("lookRight", ActuationMap{{"amount", 10.0f}})}};
  std::string bodyType = "cylinder";

  ESP_SMART_POINTERS(AgentConfiguration)
};
bool operator==(const AgentConfiguration& a, const AgentConfiguration& b);
bool operator!=(const AgentConfiguration& a, const AgentConfiguration& b);

// Represents an agent that can act within an environment
class Agent : public Magnum::SceneGraph::AbstractFeature3D {
 public:
  // constructor: the status of the agent, sensors is "valid" after
  // construction; user can use them immediately
  explicit Agent(scene::SceneNode& agentNode, const AgentConfiguration& cfg);

  virtual ~Agent();

  // Get the scene node being attached to.
  scene::SceneNode& node() { return object(); }
  const scene::SceneNode& node() const { return object(); }

  // Overloads to avoid confusion
  scene::SceneNode& object() {
    return static_cast<scene::SceneNode&>(
        Magnum::SceneGraph::AbstractFeature3D::object());
  }
  const scene::SceneNode& object() const {
    return static_cast<const scene::SceneNode&>(
        Magnum::SceneGraph::AbstractFeature3D::object());
  }

  void act(const std::string& actionName);

  void getState(AgentState::ptr state) const;

  void setState(const AgentState& state, const bool resetSensors = true);

  scene::ObjectControls::ptr getControls() { return controls_; }

  const sensor::SensorSuite& getSensorSuite() const { return sensors_; }
  sensor::SensorSuite& getSensorSuite() { return sensors_; }

  const AgentConfiguration& getConfig() const { return configuration_; }
  AgentConfiguration& getConfig() { return configuration_; }

  // Set of actions that are applied to the body of the agent.  These actions
  // update both the absolute position/rotation of the agent and the sensor
  // Non-body actions only effect the absolute position and rotation of the
  // sensors (only effects their position/rotation relative to the agent's body)
  static const std::set<std::string> BodyActions;

 private:
  AgentConfiguration configuration_;
  sensor::SensorSuite sensors_;
  scene::ObjectControls::ptr controls_;

  ESP_SMART_POINTERS(Agent)
};

}  // namespace agent
}  // namespace esp

// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "Sampler.h"

#include <Corrade/Utility/Arguments.h>
#include <Magnum/EigenIntegration/GeometryIntegration.h>
#include <Magnum/EigenIntegration/Integration.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <sophus/so3.hpp>

#include "Drawable.h"
#include "esp/io/io.h"

#include <fstream>
#include <cmath>

using namespace Magnum;
using namespace Math::Literals;
using namespace Corrade;

constexpr float moveSensitivity = 0.1f;
constexpr float lookSensitivity = 11.25f;
constexpr float cameraHeight = 1.5f;

namespace esp {
namespace gfx {

Sampler::Sampler(const Arguments& arguments)
    : Platform::Application{arguments,
                            Configuration{}.setTitle("Sampler").setWindowFlags(
                                Configuration::WindowFlag::Resizable),
                            GLConfiguration{}.setColorBufferSize(
                                Vector4i(8, 8, 8, 8))},
      pathfinder_(nav::PathFinder::create()),
      controls_(),
      previousPosition_() {
  Utility::Arguments args;
#ifdef CORRADE_TARGET_EMSCRIPTEN
  args.addNamedArgument("scene")
#else
  args.addArgument("scene")
#endif
      .setHelp("scene", "scene file to load")
      .addSkippedPrefix("magnum", "engine-specific options")
      .setGlobalHelp("Displays a 3D scene file provided on command line")
      .parse(arguments.argc, arguments.argv);

  const auto viewportSize = GL::defaultFramebuffer.viewport().size();

  // Setup renderer and shader defaults
  GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
  GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

  int sceneID = sceneManager_.initSceneGraph();
  sceneID_.push_back(sceneID);
  auto& sceneGraph = sceneManager_.getSceneGraph(sceneID);
  auto& rootNode = sceneGraph.getRootNode();
  auto& drawables = sceneGraph.getDrawables();
  const std::string& file = args.value("scene");

  // create camera file
  file_ = args.value("scene");

  std::cout << file_.length() << std::endl;

  char split;
  int position = file_.length();
  std::cout << position << std::endl;

  while (position >= 0) {
      split = file_[position];

      if (split == '/') {
          break;
      }
      position -= 1;
  }
  std::cout << position << std::endl;

  cameraFile_ = "";

  for (int i = 0; i <= position; i++) {
      std::cout << cameraFile_ << std::endl;
      cameraFile_ += file_[i];
  }

  cameraFile_ += "cameras.txt";
  std::cout << cameraFile_ << std::endl;

  const assets::AssetInfo info = assets::AssetInfo::fromPath(file);
  if (!resourceManager_.loadScene(info, &rootNode, &drawables)) {
    LOG(ERROR) << "cannot load " << file;
    std::exit(0);
  }

  // camera
  renderCamera_ = &sceneGraph.getDefaultRenderCamera();
  agentBodyNode_ = &rootNode.createChild();
  cameraNode_ = &agentBodyNode_->createChild();

  cameraNode_->translate({0.0f, cameraHeight, 0.0f});

  agentBodyNode_->translate({0.0f, 0.0f, 5.0f});

  float hfov = 90.0f;
  int width = viewportSize[0];
  int height = viewportSize[1];
  const float aspectRatio = static_cast<float>(width) / height;
  float znear = 0.01f;
  float zfar = 1000.0f;
  renderCamera_->setProjectionMatrix(width, height, znear, zfar, hfov);

  // load navmesh if available
  const std::string navmeshFilename = io::changeExtension(file, ".navmesh");
  if (io::exists(navmeshFilename)) {
    LOG(INFO) << "Loading navmesh from " << navmeshFilename;
    pathfinder_->loadNavMesh(navmeshFilename);
    LOG(INFO) << "Loaded.";
  }

  // connect controls to navmesh if loaded
  if (pathfinder_->isLoaded()) {
    controls_.setMoveFilterFunction([&](const vec3f& start, const vec3f& end) {
      vec3f currentPosition = pathfinder_->tryStep(start, end);
      LOG(INFO) << "position=" << currentPosition.transpose() << " rotation="
                << quatf(agentBodyNode_->rotation()).coeffs().transpose();
      LOG(INFO) << "Distance to closest obstacle: "
                << pathfinder_->distanceToClosestObstacle(currentPosition);
      return currentPosition;
    });

    const vec3f position = pathfinder_->getRandomNavigablePoint();
    agentBodyNode_->setTranslation(Vector3(position));

    LOG(INFO) << "Sampler initialization is done. ";
    renderCamera_->node().setTransformation(
        cameraNode_->absoluteTransformation());
  }  // namespace gfx
}

Vector3 positionOnSphere(Magnum::SceneGraph::Camera3D& camera,
                         const Vector2i& position) {
  const Vector2 positionNormalized =
      Vector2{position} / Vector2{camera.viewport()} - Vector2{0.5f};
  const Float length = positionNormalized.length();
  const Vector3 result(length > 1.0f
                           ? Vector3(positionNormalized, 0.0f)
                           : Vector3(positionNormalized, 1.0f - length));
  return (result * Vector3::yScale(-1.0f)).normalized();
}

void Sampler::drawEvent() {
  GL::defaultFramebuffer.clear(GL::FramebufferClear::Color |
                               GL::FramebufferClear::Depth);
  if (sceneID_.size() <= 0)
    return;

  int DEFAULT_SCENE = 0;
  int sceneID = sceneID_[DEFAULT_SCENE];
  auto& sceneGraph = sceneManager_.getSceneGraph(sceneID);
  renderCamera_->getMagnumCamera().draw(sceneGraph.getDrawables());
  swapBuffers();
}

void Sampler::viewportEvent(ViewportEvent& event) {
  GL::defaultFramebuffer.setViewport({{}, framebufferSize()});
  renderCamera_->getMagnumCamera().setViewport(event.windowSize());
}

void Sampler::mousePressEvent(MouseEvent& event) {
  if (event.button() == MouseEvent::Button::Left)
    previousPosition_ =
        positionOnSphere(renderCamera_->getMagnumCamera(), event.position());
}

void Sampler::mouseReleaseEvent(MouseEvent& event) {
  if (event.button() == MouseEvent::Button::Left)
    previousPosition_ = Vector3();
}

void Sampler::mouseScrollEvent(MouseScrollEvent& event) {
  if (!event.offset().y()) {
    return;
  }

  /* Distance to origin */
  const float distance =
      renderCamera_->node().transformation().translation().z();

  /* Move 15% of the distance back or forward */
  renderCamera_->node().translateLocal(
      {0.0f, 0.0f,
       distance * (1.0f - (event.offset().y() > 0 ? 1 / 0.85f : 0.85f))});

  redraw();
}

void Sampler::mouseMoveEvent(MouseMoveEvent& event) {
  if (!(event.buttons() & MouseMoveEvent::Button::Left)) {
    return;
  }

  const Vector3 currentPosition =
      positionOnSphere(renderCamera_->getMagnumCamera(), event.position());
  const Vector3 axis = Math::cross(previousPosition_, currentPosition);

  if (previousPosition_.length() < 0.001f || axis.length() < 0.001f) {
    return;
  }
  const auto angle = Math::angle(previousPosition_, currentPosition);
  renderCamera_->node().rotate(-angle, axis.normalized());
  previousPosition_ = currentPosition;

  redraw();
}

void Sampler::keyPressEvent(KeyEvent& event) {
  const auto key = event.key();
  switch (key) {
    case KeyEvent::Key::Esc:
      std::exit(0);
      break;
    case KeyEvent::Key::Left:
      controls_(*agentBodyNode_, "lookLeft", lookSensitivity);
      break;
    case KeyEvent::Key::Right:
      controls_(*agentBodyNode_, "lookRight", lookSensitivity);
      break;
    case KeyEvent::Key::Up:
      controls_(*cameraNode_, "lookUp", lookSensitivity, false);
      break;
    case KeyEvent::Key::Down:
      controls_(*cameraNode_, "lookDown", lookSensitivity, false);
      break;
    case KeyEvent::Key::Nine: {
      const vec3f position = pathfinder_->getRandomNavigablePoint();
      agentBodyNode_->setTranslation(Vector3(position));
    } break;
    case KeyEvent::Key::A:
      controls_(*agentBodyNode_, "moveLeft", moveSensitivity);
      break;
    case KeyEvent::Key::D:
      controls_(*agentBodyNode_, "moveRight", moveSensitivity);
      break;
    case KeyEvent::Key::S:
      controls_(*agentBodyNode_, "moveBackward", moveSensitivity);
      break;
    case KeyEvent::Key::W:
      controls_(*agentBodyNode_, "moveForward", moveSensitivity);
      break;
    case KeyEvent::Key::X:
      controls_(*cameraNode_, "moveDown", moveSensitivity, false);
      break;
    case KeyEvent::Key::Z:
      controls_(*cameraNode_, "moveUp", moveSensitivity, false);
      break;
    default:
      break;
  }
  renderCamera_->node().setTransformation(
      cameraNode_->absoluteTransformation());
  redraw();

  std::ofstream camera_file(cameraFile_, std::ios_base::app);

  const auto position = Magnum::EigenIntegration::cast<vec3f>(renderCamera_->node().absoluteTransformation().translation());
  const auto rotation = quatf(renderCamera_->node().rotation()).coeffs().transpose();

  std::cout << "rotation 2 " << rotation << std::endl;
  std::cout << "rotation 2 elements " << rotation[0] << " " << rotation[1] << " " << rotation[2] << " " << rotation[3] << std::endl;


    //  const auto rotation = Magnum::EigenIntegration::cast<mat3f>(renderCamera_->node().absoluteTransformation().rotation());
    //
    //  double qw, qx, qy, qz;
    //
    //  qw = std::sqrt(1. + rotation(0, 0) + rotation(1, 1) + rotation(2, 2));
    //  qx = (rotation(2, 1) - rotation(1, 2)) / (4 * qw);
    //  qy = (rotation(0, 2) - rotation(2, 0)) / (4 * qw);
    //  qz = (rotation(1, 0) - rotation(0, 1)) / (4 * qw);
    //
    //  std::cout << qw << " " << qx << " " << qy << " " << qz << std::endl;
    //
    //  std::cout << "position 1 " << position << std::endl;
    //  std::cout << rotation << std::endl;
    //  std::cout << "rotation 1 " << rotation(0, 0) << " " << rotation(0, 1) << " " << rotation(0, 2) << " " << rotation(1, 0) << " " << rotation(1, 1) << " " << rotation(1, 2) << " " << rotation(2, 0) << " " << rotation(2, 1) << " " << rotation(2, 2) << std::endl;

    //  camera_file << position[0] << " " << position[1] << " " << position[2] << " " << rotation(0, 0) << " " << rotation(0, 1) << " " << rotation(0, 2) << " " << rotation(1, 0) << " " << rotation(1, 1) << " " << rotation(1, 2) << " " << rotation(2, 0) << " " << rotation(2, 1) << " " << rotation(2, 2) << "\n";

  camera_file << position[0] << " " << position[1] << " " << position[2] << " " << " " << rotation[0] << " " << rotation[1] << " " << rotation[2] << " " << rotation[3] << "\n";
  camera_file.close();

  writeCamera(*renderCamera_);

}

void Sampler::writeCamera(RenderCamera &camera) {

    const auto position = Magnum::EigenIntegration::cast<vec3f>(camera.node().absoluteTransformation().translation());
    const auto rotation = quatf(camera.node().rotation()).coeffs().transpose();
    std::cout << "position 2" << position << std::endl;
    std::cout << "rotation 2 " << rotation << std::endl;
    std::cout << "rotation 2 elements " << rotation[0] << " " << rotation[1] << " " << rotation[2] << " " << rotation[3] << std::endl;




}

}  // namespace gfx
}  // namespace esp

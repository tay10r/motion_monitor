#include "config.h"

#include <yaml-cpp/yaml.h>

#include <nlohmann/json.hpp>

#include <spdlog/spdlog.h>

#include <set>
#include <sstream>
#include <stdexcept>

namespace {

void
load_widget_config(const YAML::Node& node, config::widget_config& w)
{
  w.label = node["label"].as<std::string>();

  w.col = node["col"].as<int>();
  w.row = node["row"].as<int>();

  w.col_span = node["col_span"].as<int>();
  w.row_span = node["row_span"].as<int>();
}

auto
load_ui_config(const YAML::Node& root) -> config::ui_config
{
  config::ui_config cfg;

  cfg.grid_cols = root["grid"]["cols"].as<int>();
  cfg.grid_rows = root["grid"]["rows"].as<int>();

  for (const auto widget_cfg : root["camera_widgets"]) {

    config::camera_widget_config cam_cfg;

    load_widget_config(widget_cfg, cam_cfg);

    cam_cfg.sensor_id = widget_cfg["sensor_id"].as<std::uint32_t>();

    cfg.camera_widgets.emplace_back(std::move(cam_cfg));
  }

  for (const auto& widget_cfg : root["microphone_widgets"]) {

    config::microphone_widget_config mic_cfg;

    load_widget_config(widget_cfg, mic_cfg);

    mic_cfg.sensor_id = widget_cfg["sensor_id"].as<std::uint32_t>();

    cfg.microphone_widgets.emplace_back(std::move(mic_cfg));
  }

  for (const auto widget_cfg : root["chart_widgets"]) {
  }

  return cfg;
}

void
load_microphones(const YAML::Node& root, config& cfg)
{
  for (const auto& node : root["microphones"]) {

    config::microphone_config mic;

    mic.name = node["name"].as<std::string>();

    mic.sensor_id = node["sensor_id"].as<std::uint32_t>();

    cfg.microphones.emplace_back(std::move(mic));
  }
}

void
load_impl(const YAML::Node& root, config& cfg)
{
  cfg.server_ip = root["server_ip"].as<std::string>(cfg.server_ip);

  cfg.tcp_server_enabled = root["tcp_server_enabled"].as<bool>(cfg.tcp_server_enabled);

  cfg.tcp_server_port = root["tcp_server_port"].as<int>(cfg.tcp_server_port);

  cfg.http_server_enabled = root["http_server_enabled"].as<bool>(cfg.http_server_enabled);

  cfg.http_server_port = root["http_server_port"].as<int>(cfg.http_server_port);

  for (const auto& node : root["cameras"]) {

    config::camera_config cam_cfg;

    cam_cfg.device_index = node["device_index"].as<int>();

    const auto exposure_mode = node["exposure_mode"].as<std::string>("");
    if ((exposure_mode == "") || (exposure_mode == "none")) {
      cam_cfg.exposure_mode = config::camera_exposure_mode::none;
    } else if (exposure_mode == "manual") {
      cam_cfg.exposure_mode = config::camera_exposure_mode::manual;
    } else if (exposure_mode == "auto") {
      cam_cfg.exposure_mode = config::camera_exposure_mode::automatic;
    } else if (exposure_mode == "gradient_maximization") {
      cam_cfg.exposure_mode = config::camera_exposure_mode::gradient_maximization;
    } else {
      spdlog::warn("Unknown camera exposure mode '{}'.", exposure_mode);
    }

    cam_cfg.name = node["name"].as<std::string>();

    cam_cfg.jpeg_quality = node["stream_quality"].as<float>();

    const auto frame_size = node["size"];
    if (frame_size.IsDefined() && frame_size.IsNull()) {
      cam_cfg.frame_width = frame_size["width"].as<int>();
      cam_cfg.frame_height = frame_size["height"].as<int>();
    }

    const auto& people_detection = node["people_detection"];
    cam_cfg.people_detection_enabled = people_detection["enabled"].as<bool>();

    const auto& storage = node["storage"];
    cam_cfg.storage_enabled = storage["enabled"].as<bool>();
    cam_cfg.storage_quality = storage["quality"].as<float>();
    cam_cfg.storage_path = storage["directory"].as<std::string>(".");
    cam_cfg.storage_days = storage["days"].as<float>();
    const auto storage_size = storage["size"];
    if (storage_size.IsDefined() && !storage_size.IsNull()) {
    }

    cfg.cameras.emplace_back(std::move(cam_cfg));
  }

  load_microphones(root, cfg);

  if (root["landscape_ui"]) {
    cfg.landscape_ui = load_ui_config(root["landscape_ui"]);
  }

  if (root["portrait_ui"].IsDefined()) {
    cfg.portrait_ui = load_ui_config(root["portait_ui"]);
  } else {
    cfg.portrait_ui = cfg.landscape_ui;
  }
}

} // namespace

void
config::load_string(const char* str)
{
  const auto root = YAML::Load(str);

  load_impl(root, *this);
}

void
config::load(const char* path)
{
  const auto root = YAML::LoadFile(path);

  load_impl(root, *this);
}

namespace {

void
export_widget_config(const config::widget_config& cfg, nlohmann::json& root)
{
  root["label"] = cfg.label;
  root["row"] = cfg.row;
  root["col"] = cfg.col;
  root["row_span"] = cfg.row_span;
  root["col_span"] = cfg.col_span;
}

auto
export_ui_config(const config::ui_config& cfg) -> nlohmann::json
{
  nlohmann::json root;

  {
    nlohmann::json grid;
    grid["rows"] = cfg.grid_rows;
    grid["cols"] = cfg.grid_cols;
    root["grid"] = grid;
  }

  std::vector<nlohmann::json> camera_widgets;

  for (const auto& cam : cfg.camera_widgets) {

    nlohmann::json camera;

    camera["sensor_id"] = cam.sensor_id;

    export_widget_config(cam, camera);

    camera_widgets.emplace_back(std::move(camera));
  }

  root["camera_widgets"] = std::move(camera_widgets);

  std::vector<nlohmann::json> microphone_widgets;

  for (const auto& mic : cfg.microphone_widgets) {

    nlohmann::json node;

    node["sensor_id"] = mic.sensor_id;

    export_widget_config(mic, node);

    microphone_widgets.emplace_back(std::move(node));
  }

  root["microphone_widgets"] = microphone_widgets;

  std::vector<nlohmann::json> chart_widgets;

  /* TODO */

  root["chart_widgets"] = std::move(chart_widgets);

  return root;
}

} // namespace

auto
config::export_dashboard_config() const -> std::string
{
  nlohmann::json root;

  root["landscape_ui"] = export_ui_config(landscape_ui);

  root["portrait_ui"] = export_ui_config(portrait_ui);

  return root.dump();
}

namespace {

template<typename Item, typename Accessor>
void
check_unique_names(const std::vector<Item>& items, Accessor accessor)
{
  std::vector<std::string> names;

  for (const auto& it : items) {
    names.emplace_back(accessor(it));
  }

  std::set<std::string> unique_names;

  for (const auto& name : names) {

    if (unique_names.find(name) != unique_names.end()) {
      std::ostringstream stream;
      stream << "The name '" << name << "' is already used.";
      throw std::runtime_error(stream.str());
    }

    unique_names.emplace(name);
  }
}

} // namespace

void
config::validate() const
{
  check_unique_names(cameras, [](const camera_config& cfg) -> std::string { return cfg.name; });

  check_unique_names(microphones, [](const microphone_config& cfg) -> std::string { return cfg.name; });
}


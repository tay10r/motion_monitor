#include "config.h"

#include <yaml-cpp/yaml.h>

#include <nlohmann/json.hpp>

#include <set>
#include <sstream>
#include <stdexcept>

namespace {

auto
load_ui_config(const YAML::Node& root) -> config::ui_config
{
  config::ui_config cfg;

  cfg.grid_cols = root["grid"]["cols"].as<int>();
  cfg.grid_rows = root["grid"]["rows"].as<int>();

  for (const auto widget_cfg : root["camera_widgets"]) {

    config::camera_widget_config cam_cfg;

    cam_cfg.label = widget_cfg["label"].as<std::string>();

    cam_cfg.sensor_id = widget_cfg["sensor_id"].as<std::uint32_t>();

    cam_cfg.col = widget_cfg["col"].as<int>();
    cam_cfg.row = widget_cfg["row"].as<int>();

    cam_cfg.col_span = widget_cfg["col_span"].as<int>();
    cam_cfg.row_span = widget_cfg["row_span"].as<int>();

    cfg.camera_widgets.emplace_back(std::move(cam_cfg));
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
    cam_cfg.name = node["name"].as<std::string>();
    cam_cfg.read_interval = node["read_interval"].as<int>(1000);
    cam_cfg.detector_path = node["detector_path"].as<std::string>("");

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


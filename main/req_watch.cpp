// ARMOR-NODE-RADAR - watches the requests of the panel's servers (core/slow_req.hpp) and says the one that does not end.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include "req_watch.hpp"

#include <mutex>
#include <string>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "core/slow_req.hpp"
#include "psram_mem.hpp"

namespace armor::reqwatch {
namespace {

constexpr char kTag[] = "armor-web";

std::mutex g_requests_lock;
slowreq::Slot g_requests[2];   // one for each server: plain and HTTPS

std::uint64_t now_ms() { return static_cast<std::uint64_t>(esp_timer_get_time()) / 1000ULL; }

// A task of its own: the server's task is the one that does not come back. It touches no flash (a heap query and a log line), so its stack can be in the external RAM.
void request_watch_task(void*) {
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(5000));
    for (slowreq::Slot& slot : g_requests) {
      std::string what;
      std::uint64_t ms = 0;
      {
        std::lock_guard<std::mutex> guard(g_requests_lock);
        if (!slot.stuck(now_ms())) continue;
        what = slot.what;
        ms = now_ms() - slot.since_ms;
      }
      const membudget::Snapshot memory = memory::snapshot();
      ESP_LOGE(kTag, "%s", slowreq::line(what, ms, true, memory.internal_free, memory.internal_largest).c_str());   // an error line: the health report carries it to the server
    }
  }
}

}  // namespace

void start() {
  static bool started = false;
  if (started) return;
  started = memory::start_task(request_watch_task, "web-watch", 4096, nullptr, 6);   // above the server's own priority, so that a busy server does not hide itself
}

esp_err_t run(httpd_req_t* r, bool tls, esp_err_t (*handler)(httpd_req_t*)) {
  slowreq::Slot& slot = g_requests[tls ? 1 : 0];
  const char* const method = r->method == HTTP_GET ? "GET" : r->method == HTTP_POST ? "POST" : r->method == HTTP_PUT ? "PUT" : "DELETE";
  const std::string what = slowreq::label(method, r->uri);
  {
    std::lock_guard<std::mutex> guard(g_requests_lock);
    slot.begin(what, now_ms());
  }
  const esp_err_t result = handler(r);
  std::uint64_t took = 0;
  {
    std::lock_guard<std::mutex> guard(g_requests_lock);
    took = slot.end(now_ms());
  }
  if (took >= slowreq::kSlowMs && !slowreq::may_be_long(what)) {
    const membudget::Snapshot memory = memory::snapshot();
    ESP_LOGW(kTag, "%s", slowreq::line(what, took, false, memory.internal_free, memory.internal_largest).c_str());
  }
  return result;
}

}  // namespace armor::reqwatch

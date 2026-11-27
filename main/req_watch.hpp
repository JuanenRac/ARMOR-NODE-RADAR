// ARMOR-NODE-RADAR - watches the requests of the panel's servers (core/slow_req.hpp) and says the one that does not end.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include "esp_http_server.h"

namespace armor::reqwatch {

// Starts the task that looks at the requests that are running. A second call does nothing.
void start();

// Runs `handler` for one request and keeps note of it meanwhile. `tls` says which of the two servers is answering: each one answers one request at a time.
esp_err_t run(httpd_req_t* r, bool tls, esp_err_t (*handler)(httpd_req_t*));

}  // namespace armor::reqwatch

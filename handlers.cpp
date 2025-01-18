#include "AsyncCam.hpp"
#include <StreamString.h>

static const char FRONTPAGE[] = R"EOT(
<!doctype html>
<title>ESP32-CAM with ability to post camera images to a server</title>
<body>
<div style="display: flex; align-items: center;">
  <img src="https://cdn-icons-png.flaticon.com/512/1042/1042390.png" alt=" " style="width:50px;height:50px; margin-right: 10px;">
  <h1>ESP32-CAM SnapSend</h1>
</div>
<form><p>
Resolution
<select id="resolution" required>%RESOLUTION_OPTIONS%</select>
<input type="submit" value="change">
</p></form>
<p id="controls">
<button data-act="mjpeg">show Motion JPEG stream</button>
<button data-act="jpg">show still JPEG image</button>
<button id="uploadCapture">send image to server</button>
<button data-act="">hide</button>
</p>
<div id="display"></div>
<footer>By <a href="https://github.com/komkritc?tab=repositories">Komkrit Chooruang</a></footer>

<script type="module">
async function fetchText(uri, init) {
  const response = await fetch(uri, init);
  if (!response.ok) {
    throw new Error(await response.text());
  }
  return (await response.text()).trim().replaceAll("\r\n", "\n");
}

const $display = document.querySelector("#display");
const $resolution = document.querySelector("#resolution");

// Handle resolution change
$resolution.form.addEventListener("submit", async (evt) => {
  evt.preventDefault();
  const [width, height] = $resolution.value.split("x");
  try {
    const change = await fetchText("/change-resolution.cgi", {
      method: "POST",
      body: new URLSearchParams({ width, height }),
    });
  } catch (err) {
    $display.textContent = err.toString();
  }
});

// Handle camera controls
for (const $ctrl of document.querySelectorAll("#controls button[data-act]")) {
  $ctrl.addEventListener("click", (evt) => {
    evt.preventDefault();

    const $img = $display.querySelector("img");
    if ($img) {
      $img.src = "";
    }

    const act = evt.target.getAttribute("data-act");
    if (act === "") {
      $display.innerHTML = "";
    } else {
      $display.innerHTML = `<img src="/cam.${act}?_=${Math.random()}" alt="camera image">`;
    }
  });
}

// Handle sending still capture to Django server
document.querySelector("#uploadCapture").addEventListener("click", async (evt) => {
  evt.preventDefault();

  const $display = document.querySelector("#display");

  try {
    // Fetch the still image from the ESP32 camera
    const response = await fetch("/cam.jpg");
    if (!response.ok) {
      throw new Error(`Failed to capture image: ${response.statusText}`);
    }

    // Convert the image into a Blob
    const imageBlob = await response.blob();

    // Prepare form data for upload
    const formData = new FormData();
    formData.append("image", imageBlob, "capture.jpg");

    // Send the image to the Django server
    const uploadResponse = await fetch("http://192.168.1.155:8000/reporting/upload/", {
      method: "POST",
      body: formData,
    });

    if (!uploadResponse.ok) {
      throw new Error(`Upload failed: ${uploadResponse.statusText}`);
    }

    const result = await uploadResponse.text();
    $display.innerHTML = `<p>Upload successful! Server response: ${result}</p>`;
  } catch (error) {
    $display.innerHTML = `<p>Error: ${error.message}</p>`;
  }
});

// Handle manual file upload form submission
const uploadForm = document.querySelector("#uploadForm");

uploadForm.addEventListener("submit", async (event) => {
  event.preventDefault(); // Prevent default form submission

  const formData = new FormData(uploadForm);
  const display = document.querySelector("#display");

  try {
    const response = await fetch(uploadForm.action, {
      method: "POST",
      body: formData,
    });

    if (!response.ok) {
      throw new Error(`Upload failed: ${response.statusText}`);
    }

    const result = await response.text();
    display.innerHTML = `<p>Upload successful! Server response: ${result}</p>`;
  } catch (error) {
    display.innerHTML = `<p>Error: ${error.message}</p>`;
  }
});

</script>
)EOT";

static String
rewriteFrontpage(const String& var) {
  StreamString b;
  if (var == "RESOLUTION_OPTIONS") {
    for (const auto& r : esp32cam::Camera.listResolutions()) {
      b.print("<option");
      if (r == currentResolution) {
        b.print(" selected");
      }
      if (r > initialResolution) {
        b.print(" disabled");
      }
      b.print('>');
      b.print(r);
      b.print("</option>");
    }
  }
  return b;
}

void
addRequestHandlers() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", reinterpret_cast<const uint8_t*>(FRONTPAGE),
                    sizeof(FRONTPAGE), rewriteFrontpage);
  });

  server.on("/robots.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", "User-Agent: *\nDisallow: /\n");
  });

  server.on("/change-resolution.cgi", HTTP_POST, [](AsyncWebServerRequest* request) {
    long width = request->arg("width").toInt();
    long height = request->arg("height").toInt();
    auto r = esp32cam::Camera.listResolutions().find(width, height);
    if (!(r.isValid() && r.getWidth() == width && r.getHeight() == height)) {
      request->send(404, "text/plain", "non-existent resolution\n");
      return;
    }

    if (!esp32cam::Camera.changeResolution(r, 0)) {
      Serial.printf("changeResolution(%ld,%ld) failure\n", width, height);
      request->send(500, "text/plain", "changeResolution error\n");
      return;
    }

    currentResolution = r;
    Serial.printf("changeResolution(%ld,%ld) success\n", width, height);
    StreamString b;
    b.print(currentResolution);
    request->send(b, "text/plain", b.length());
  });

  server.on("/cam.jpg", esp32cam::asyncweb::handleStill);
  server.on("/cam.mjpeg", esp32cam::asyncweb::handleMjpeg);
}
